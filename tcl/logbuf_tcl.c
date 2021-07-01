#include <stdint.h>
#include <malloc.h>
#include <string.h>
#include "logbuf.h"
#include "logdest.h"
#include "logres.h"
#include <tcl.h>
#include <tclTomMath.h>

static int logres_obj_init(Tcl_Interp *interp, Tcl_Obj *o);
static void logres_obj_dup(Tcl_Obj *src, Tcl_Obj *dst);
static void logres_obj_update_string(Tcl_Obj *o);
static void logres_obj_free(Tcl_Obj *o);

static struct Tcl_ObjType logres_objtype = {
 .name = "logres",
 .freeIntRepProc = logres_obj_free,
 .dupIntRepProc = logres_obj_dup,
 .updateStringProc = logres_obj_update_string,
 .setFromAnyProc = logres_obj_init
};

static int
logres_obj_init(Tcl_Interp *interp, Tcl_Obj *o)
{
  const char *str;
  logres_parse_status_t stat;
  logres_t *res;
  uint32_t errp;

  str = Tcl_GetString(o);
  res = logres_init();
  stat = logres_parse_ini(res, str, &errp);
  if(stat == LOGRES_PARSE_OK) {
   const Tcl_ObjType *old_type = o->typePtr;

   if(old_type && old_type->freeIntRepProc != NULL) 
    old_type->freeIntRepProc(o);
   o->internalRep.otherValuePtr = res;
   o->typePtr = &logres_objtype;
   return TCL_OK;
  }

  if(interp != NULL) {
    char buf[100];
    sprintf(buf, "parsing of ini failed at offset %u with error %u", errp, stat);
	Tcl_ResetResult(interp);
	Tcl_AppendToObj(Tcl_GetObjResult(interp), buf, -1);            
  }
  return TCL_ERROR;
  
}

static void
logres_obj_dup(Tcl_Obj *src, Tcl_Obj *dst)
{
 dst->typePtr = NULL;
}

static void
logres_obj_free(Tcl_Obj *o)
{
 logres_free(o->internalRep.otherValuePtr);
}

static void
logres_obj_update_string(Tcl_Obj *o)
{
#define UNIMPL "<unimplemented>"
 char *s = ckalloc(sizeof(UNIMPL));
 sprintf(s, UNIMPL);
 o->bytes = s;
 o->length = sizeof(UNIMPL)-1;
}

static const char *type_names[23] = {
 [0] = "", 
 [LOGBUF_T_MID] = "MID",
 [LOGBUF_T_GRP] = "GRP",
 [LOGBUF_T_I32] = "I32",
 [LOGBUF_T_I64] = "I64",
 [LOGBUF_T_STR] = "STR",
 [LOGBUF_T_DATA] = "DATA",
 [LOGBUF_T_FMT] = "FMT",
 [LOGBUF_T_TIME] = "TIME",
 [LOGBUF_T_RID] = "RID"
};

static const char *type2str(uint8_t t) {
 const char *tt;
 if(t > 22) return "";
 tt = type_names[t];
 if(!tt) return "";
 return tt;
}

static uint8_t str2type(const char *t) {
 int i;

 for(i = 0; i <= 22; i++) {
  if(!type_names[i]) continue;
  if(!strcmp(t, type_names[i])) return i;
 }
 return 255;
}

static int 
tcl_logdest_get_arg_common(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[], int need_info)
{
 const uint8_t *lb;
 Tcl_Obj *o;
 Tcl_Obj *res[3];
 int lblen;
 int t, n = 0;
 const void *ptr;
 uint32_t len;
 uint8_t type;
 uint32_t arg;
 int use_str_type;

 if (objc < 3) {
  Tcl_WrongNumArgs (interp, 1, objv, "lb typeId ?argNum?");
  return TCL_ERROR;
 }
 lb = Tcl_GetByteArrayFromObj(objv[1], &lblen);

 t = str2type(Tcl_GetString(objv[2]));
 if(t != 255) {
  use_str_type = 1;
 } else {
  if(Tcl_GetIntFromObj(interp, objv[2], &t)) return TCL_ERROR;
  use_str_type = 0;
 }

 if(objc > 3) if(Tcl_GetIntFromObj(interp, objv[3], &n)) return TCL_ERROR;
 if(!logdest_get_arg((const uint8_t*)lb, lblen, t, n, &ptr, &len, &type, &arg)) {
  Tcl_SetResult(interp, "arg not found", TCL_STATIC);
  return TCL_ERROR;
 }
 switch(type) {
         case LOGBUF_T_MID:
         case LOGBUF_T_I32:
         case LOGBUF_T_RID:
                 o = Tcl_NewWideIntObj(*(uint32_t*)ptr);
                 break;
         case LOGBUF_T_I64: 
         case LOGBUF_T_TIME: 
         case LOGBUF_T_GRP: {
                  uint64_t i;
                  i = *(uint64_t*)ptr;
                  if(i >= 0) o = Tcl_NewWideIntObj(i);
                  else {
                   mp_int bn;
                   TclBNInitBignumFromWideUInt(&bn, i);
                   Tcl_NewBignumObj(&bn);
                   mp_clear(&bn);
                  }
                 }
                 break;
         case LOGBUF_T_STR: 
         case LOGBUF_T_FMT: 
                 o = Tcl_NewStringObj(ptr, len-1);
                 break;
         case LOGBUF_T_DATA: 
                 o = Tcl_NewByteArrayObj(ptr, len);
                 break;
         default:
                 Tcl_SetResult(interp, "unknown arg type in buf", TCL_STATIC);
                 return TCL_ERROR;
 }
 if(!need_info) {
   Tcl_SetObjResult(interp, o);
   return TCL_OK;
 }
 res[0] = o;
 if(use_str_type) res[1] = Tcl_NewStringObj(type2str(type), -1);
 else res[1] = Tcl_NewIntObj(type);
 res[2] = Tcl_NewIntObj(arg);
 Tcl_SetObjResult(interp, Tcl_NewListObj(3, res));
 return TCL_OK;
}

static int 
tcl_logdest_get_arg(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
 return tcl_logdest_get_arg_common(clientData, interp, objc, objv, 0);
}

static int 
tcl_logdest_get_arg_info(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
 return tcl_logdest_get_arg_common(clientData, interp, objc, objv, 1);
}

static int 
tcl_logdest_format_message(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
 const uint8_t *lb;
 int lblen;
 char *res;
 Tcl_Obj *o;
 int argn = 1;
 logres_t *resdb = NULL;
 const char *fmtstr = NULL;
 const char *s;

 if (objc < 2) {
  Tcl_WrongNumArgs (interp, 1, objv, "?-res resDb? ?fmtStr? lb");
  return TCL_ERROR;
 }
 o =  objv[argn];
 s = Tcl_GetString(o);
 if(s[0] == '-') {
  argn++;
  if(!strcmp(s, "-res")) {
   if(argn == objc) {
    Tcl_WrongNumArgs (interp, argn, objv, "resDb ?fmtStr? lb");
    return TCL_ERROR;
   }
   o = objv[argn++];
   if(Tcl_ConvertToType(interp, o, &logres_objtype) != TCL_OK) return TCL_ERROR;
   resdb = o->internalRep.otherValuePtr;
  } else if(strcmp(s, "--")) {
   char buf[100];
   sprintf(buf, "unexpected option \"%s\"", s);
   Tcl_ResetResult(interp);
   Tcl_AppendToObj(Tcl_GetObjResult(interp), buf, -1);            
   return TCL_ERROR;
  } 
 }
 
 if(argn == objc) {
  Tcl_WrongNumArgs (interp, argn, objv, "?fmtStr? lb");
  return TCL_ERROR;
 }
 if(argn < objc-1) 
  fmtstr = Tcl_GetString(objv[argn++]);

 lb = Tcl_GetByteArrayFromObj(objv[argn], &lblen);
 logdest_format_message(resdb, fmtstr, lb, lblen, &res);
 o = Tcl_NewStringObj(res, -1);
 free(res);
 Tcl_SetObjResult(interp, o);
 return TCL_OK;
}


int
Tcllogbuf_Init(Tcl_Interp *interp)
{
   if (!Tcl_InitStubs(interp, "8.6", 0)) {
		return TCL_ERROR;
   }

   if (Tcl_PkgProvide(interp, "Tcllogbuf", "1.0") == TCL_ERROR) {
        return TCL_ERROR;
   }
   Tcl_CreateObjCommand (interp, "logbuf_get_arg", tcl_logdest_get_arg, NULL, (Tcl_CmdDeleteProc *)NULL);
   Tcl_CreateObjCommand (interp, "logbuf_get_arg_info", tcl_logdest_get_arg_info, NULL, (Tcl_CmdDeleteProc *)NULL);
   Tcl_CreateObjCommand (interp, "logbuf_format_message", tcl_logdest_format_message, NULL, (Tcl_CmdDeleteProc *)NULL);

   return TCL_OK;
}
