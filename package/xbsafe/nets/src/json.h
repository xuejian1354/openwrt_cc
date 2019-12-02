#ifndef JSON_H
#define JSON_H

#include <pthread.h>
#include "duktape.h"

#ifdef __cplusplus
extern "C" {
#endif



int jsonInitialize(duk_context **ctx,duk_idx_t *indx);
int jsonDestroy(duk_context *ctx);
int jsonTmpObjRemove(duk_context *ctx,duk_idx_t idx);

int getInt(duk_context *ctx,const char *jsonString, const char *key);
unsigned long getNum(duk_context *ctx,const char *jsonString, const char *key);
const char *getString(duk_context *ctx,const char *jsonString, const char *key,char *resBuf);
const char *decodeBase64String(duk_context *ctx,const char *jsonString, const char *key,char *resBuf,unsigned long *len);
unsigned long getStringLen(duk_context *ctx,const char *jsonString, const char *key);
duk_ret_t delInt(duk_context *ctx,duk_idx_t indx,const char *key);
duk_ret_t putInt(duk_context *ctx,duk_idx_t indx,const char *key, const int value);
duk_ret_t putDouble(duk_context *ctx,duk_idx_t indx,const char *key, const double value);
duk_ret_t putString(duk_context *ctx,duk_idx_t indx,const char *key, const char *value);
duk_ret_t putIntToString(duk_context *ctx,duk_idx_t indx,const char *key, const int value);
duk_ret_t putUIntToString(duk_context *ctx,duk_idx_t indx,const char *key, const unsigned int value);
duk_ret_t putDoubleToString(duk_context *ctx,duk_idx_t indx,const char *key, const double value);
duk_ret_t putObjProp(duk_context *ctx,duk_idx_t indx,const char *obj);
duk_ret_t putArrayProp(duk_context *ctx,duk_idx_t indx,const char *obj);
duk_ret_t putArrayInt(duk_context *ctx,duk_idx_t indx,const char *obj,int arridx,const char *key, const int value);
duk_ret_t putArrayString(duk_context *ctx,duk_idx_t indx,const char *obj,int arridx,const char *key, const char *value);
duk_ret_t putArrayIndexObj(duk_context *ctx,duk_idx_t indx,int idx,duk_idx_t *array_obj);
duk_ret_t putArrayInt2(duk_context *ctx,duk_idx_t indx,const char *obj,int arridx,const int value);
duk_idx_t getObj(duk_context *ctx,duk_idx_t indx,const char *obj);

duk_idx_t putObj(duk_context *ctx);
duk_ret_t putObjInt(duk_context *ctx,duk_idx_t indx,const char *obj,const char *key, const int value);
duk_ret_t putObjDouble(duk_context *ctx,duk_idx_t indx,const char *obj,const char *key, const double value);
duk_ret_t putObjULLONG(duk_context *ctx,duk_idx_t indx,const char *obj,const char *key, const unsigned long long value);
duk_ret_t putObjString(duk_context *ctx,duk_idx_t indx,const char *obj,const char *key, const char *value);
int getObjInt(duk_context *ctx,const char *jsonString,const char *obj, const char *key);
const char *getObjString(duk_context *ctx,const char *jsonString,const char *obj, const char *key,char *resBuf);

//json中的数组,
duk_ret_t loadJsonArray(duk_context *ctx,const char *jsonString,const char *obj, const char *arrayName,int *arrLen, duk_idx_t *arridx);//返回数组长度
duk_ret_t loadJsonArrayEx(duk_context *ctx,const char *jsonString,const char *arrayName,int *arrLen, duk_idx_t *arridx);//返回数组长度

duk_ret_t unloadJsonArray(duk_context *ctx,duk_idx_t arridx);
const char *getArrayObjString(duk_context *ctx, duk_idx_t arr_pos,int arridx, const char *key,char *result);

int getArrayObjInt(duk_context *ctx, duk_idx_t arr_pos,int arridx,const char *key);
//索引位置不再是对象，直接获取值
duk_ret_t loadArrayObjArray(duk_context *ctx, duk_idx_t arr_pos,int arridx, const char *subArrayName,int *arrlen,duk_idx_t *subarridx);
duk_ret_t unloadJsonArrayObjArray(duk_context *ctx,duk_idx_t arridx);

int getArrayInt(duk_context *ctx,duk_idx_t indx,int arridx);
const char * getArrayString(duk_context *ctx,duk_idx_t indx,int arridx,char *value);

//生成响应结果
const char *generateResult(duk_context *ctx,duk_idx_t indx,unsigned long id);



//获取方法对应的模块id
int FuncInitialize();
int FuncDestroy();
int getFunctionSocket(const char *functionName);

//map函数集
int mapInitialize();
int mapDestroy();
duk_ret_t mapSetInt(const char *key, int value);
duk_ret_t mapSetString(const char *key, const char *value) ;
duk_ret_t mapDel(const char *key);
int mapGet(const char *key);

#ifdef __cplusplus
}
#endif

#endif