//gcc duktape.c json.c -o json -lm -lpthread -Wall
#include "json.h"
#include <stdlib.h>
#define MAX_LENGTH (256)
#define MAP
#define TEST_GET
#define TEST_SET

#ifdef MAP
static duk_context *contextMap = NULL;
static duk_idx_t indexMap = 0;
#endif
static pthread_mutex_t mutexMap = PTHREAD_MUTEX_INITIALIZER;


int jsonInitialize(duk_context **ctx,duk_idx_t *indx)
{
	*ctx = duk_create_heap_default();
	if (NULL == *ctx) {
		fprintf(stderr, "duk_create_heap_default() Failed!\n");
		return -1;
	}
	else {
		*indx = duk_push_object(*ctx);
	}
	return 0;
}
int jsonDestroy(duk_context *ctx)
{
	if(ctx != NULL)
		duk_destroy_heap(ctx);
	return 0;
}

int jsonTmpObjRemove(duk_context *ctx,duk_idx_t idx)
{
	if(idx != DUK_INVALID_INDEX)
	{
		duk_remove(ctx,idx);
	}
	
	return 0;
}

#ifdef TEST_GET
int getInt(duk_context *ctx,const char *jsonString, const char *key) {
  //printf("%s,json:%s,key:%s\n",__FUNCTION__,jsonString,key);
  int result = -1;
  duk_push_string(ctx, jsonString);
  duk_json_decode(ctx, -1);
  if (duk_has_prop_string(ctx, -1,key)) {
	printf("Found!\n");
    duk_get_prop_string(ctx, -1, key);
	result = duk_to_int(ctx, -1);

    duk_pop_2(ctx);
  }
  else {
	printf("No Found!\n");
    result = 2147483647;
    duk_pop(ctx);
  }
  return result;
}

unsigned long getNum(duk_context *ctx,const char *jsonString, const char *key)
{
	unsigned long result = 2147483647;
	duk_push_string(ctx, jsonString);
	duk_json_decode(ctx, -1);
	duk_push_string(ctx, key);
	if (duk_has_prop(ctx, -2)) {
		duk_get_prop_string(ctx, -1, key);
		//result = duk_to_number(ctx, -1);
		if(duk_is_number(ctx,-1))
		{
			result = (unsigned long)duk_to_number(ctx, -1);
		}else if(duk_is_string(ctx,-1))
		{
			result = (unsigned long)strtoul(duk_to_string(ctx, -1),NULL,10);
		}else
		{
			 result = 2147483647;
		}
		duk_pop_2(ctx);
	}
	else {
		duk_pop(ctx);
	}
	return result;
}

const char *getString(duk_context *ctx,const char *jsonString, const char *key,char *resBuf) {
  char *result = resBuf;
  duk_push_string(ctx, jsonString);
  duk_json_decode(ctx, -1);
  duk_push_string(ctx, key);
  if (duk_has_prop(ctx, -2)) {
    duk_get_prop_string(ctx, -1, key);
  //当key 不存在时，返回“undefined ”
    strcpy(resBuf,duk_to_string(ctx, -1));
    duk_pop_2(ctx);
  }
  else {
    result = NULL;
    duk_pop(ctx);
  }
  return result;
}

unsigned long getStringLen(duk_context *ctx,const char *jsonString, const char *key) {
  duk_size_t len = 0;
  
  duk_push_string(ctx, jsonString);
  duk_json_decode(ctx, -1);
  duk_push_string(ctx, key);
  if (duk_has_prop(ctx, -2)) {
    duk_get_prop_string(ctx, -1, key);
	duk_get_lstring(ctx,-1,&len);
    duk_pop_2(ctx);
  }
  else {
    duk_pop(ctx);
  }
  return (unsigned long)len;
}

const char *decodeBase64String(duk_context *ctx,const char *jsonString, const char *key,char *resBuf,unsigned long *len)
{
	*len = 0;
	const char *result = NULL;
	duk_push_string(ctx, jsonString);
	duk_json_decode(ctx, -1);
	if (!duk_has_prop_string(ctx, -1,key)) {
		duk_pop(ctx);
		return result;
	}
	
	duk_get_prop_string(ctx, -1, key);
  	duk_base64_decode(ctx,-1);
	result = duk_to_string(ctx,-1);
	if(resBuf != NULL)
	{
		strcpy(resBuf,result);
		result = resBuf;
	}
	
	*len = strlen(result);
	
	duk_pop_2(ctx);
	
	return result;
}

duk_idx_t getObj(duk_context *ctx,duk_idx_t indx,const char *obj)
{
	duk_get_prop_string(ctx,indx,obj);
	return duk_get_top_index(ctx);
}

int getObjInt(duk_context *ctx,const char *jsonString,const char *obj, const char *key)
{
	int result = -1;
	duk_push_string(ctx, jsonString);
	duk_json_decode(ctx, -1);
	if (duk_has_prop_string(ctx, -1, obj)) {
		duk_get_prop_string(ctx, -1, obj);
		if(duk_has_prop_string(ctx, -1, key))
		{
			duk_get_prop_string(ctx, -1, key);
			//当key 不存在时，返回“0 ”
			result = duk_to_int(ctx, -1);
			duk_pop_3(ctx);
		}else
		{
			result = 2147483647;
			duk_pop_2(ctx);
		}		
	}
	else {
		//printf("No Found!\n");
		result = 2147483647;
		duk_pop(ctx);
	}
	return result;
}
const char *getObjString(duk_context *ctx,const char *jsonString,const char *obj, const char *key,char *resBuf)
{
	const char *result = resBuf;
	duk_push_string(ctx, jsonString);
	duk_json_decode(ctx, -1);
	if (duk_has_prop_string(ctx, -1, obj)) {
		duk_get_prop_string(ctx, -1, obj);
		if(duk_has_prop_string(ctx, -1, key))
		{
			duk_get_prop_string(ctx, -1, key);
			//当key 不存在时，返回“undefined ”
			strcpy(resBuf,duk_to_string(ctx, -1));
			duk_pop_3(ctx);
		}else
		{
			result = NULL;
			duk_pop_2(ctx);
		}		
		
	}
	else {
		result = NULL;
		duk_pop(ctx);
	}
	return result;
}


duk_ret_t loadJsonArray(duk_context *ctx,const char *jsonString,const char *obj, const char *arrayName,int *arrLen, duk_idx_t *arridx)
{
	duk_push_string(ctx, jsonString);
	duk_json_decode(ctx, -1);
	if (duk_has_prop_string(ctx, -1,obj) == 0) {
		
		*arrLen = 0;
		*arridx = DUK_INVALID_INDEX;
		duk_pop(ctx);
		return 0;
	}
	duk_get_prop_string(ctx, -1, obj);
	if (duk_has_prop_string(ctx, -1,arrayName) == 0) {
		
		*arrLen = 0;
		*arridx = DUK_INVALID_INDEX;
		duk_pop_2(ctx);
		return 0;
	}
	duk_get_prop_string(ctx, -1, arrayName);
	if(duk_is_array(ctx,-1) == 0)
	{
		*arrLen = 0;
		*arridx = DUK_INVALID_INDEX;
		duk_pop_3(ctx);
		return 0;
	}
	*arrLen = duk_get_length(ctx,-1);
	*arridx = duk_get_top_index(ctx);
	return 1;
}

duk_ret_t loadJsonArrayEx(duk_context *ctx,const char *jsonString,const char *arrayName,int *arrLen, duk_idx_t *arridx)
{
	duk_push_string(ctx, jsonString);
	duk_json_decode(ctx, -1);
	if (duk_has_prop_string(ctx, -1,arrayName) == 0) {
		
		*arrLen = 0;
		*arridx = DUK_INVALID_INDEX;
		duk_pop(ctx);
		return 0;
	}
	duk_get_prop_string(ctx, -1, arrayName);
	if(duk_is_array(ctx,-1) == 0)
	{
		*arrLen = 0;
		*arridx = DUK_INVALID_INDEX;
		duk_pop_2(ctx);
		return 0;
	}
	*arrLen = duk_get_length(ctx,-1);
	*arridx = duk_get_top_index(ctx);
	return 1;	
}

const char *getArrayObjString(duk_context *ctx, duk_idx_t arr_pos,int arridx, const char *key,char *result)
{
	if(duk_has_prop_index(ctx,arr_pos,arridx) == 0)
		return NULL;
	
	duk_get_prop_index(ctx,arr_pos,arridx);
	if(duk_has_prop_string(ctx,-1,key) == 0)
	{
		duk_pop(ctx);
		return NULL;
	}
	duk_get_prop_string(ctx,-1,key);
	
	strcpy(result,duk_to_string(ctx,-1));
	duk_pop_2(ctx);
	return result;
}

duk_ret_t loadArrayObjArray(duk_context *ctx, duk_idx_t arr_pos,int arridx, const char *subArrayName,int *arrlen,duk_idx_t *subarridx)
{
	if(!duk_has_prop_index(ctx,arr_pos,arridx))
		return 0;
	
	duk_get_prop_index(ctx,arr_pos,arridx);
	if(!duk_has_prop_string(ctx,-1,subArrayName))
	{
		duk_pop(ctx);
		return 0;
	}
	
	duk_get_prop_string(ctx,-1,subArrayName);
	if(duk_is_array(ctx,-1) == 0)
	{
		*arrlen = 0;
		*subarridx = DUK_INVALID_INDEX;
		duk_pop_2(ctx);
		return 0;
	}
	
	*arrlen = duk_get_length(ctx,-1);
	*subarridx = duk_get_top_index(ctx);
	return 1;
}

int getArrayObjInt(duk_context *ctx, duk_idx_t arr_pos,int arridx,const char *key)
{
	int ret = 0;
	if(duk_has_prop_index(ctx,arr_pos,arridx) == 0)
		return 2147483647;
	
	duk_get_prop_index(ctx,arr_pos,arridx);
	if(duk_has_prop_string(ctx,-1,key) == 0)
	{
		duk_pop(ctx);
		return 2147483647;
	}
	duk_get_prop_string(ctx,-1,key);
	ret = duk_to_int(ctx,-1);
	duk_pop_2(ctx);
	return ret;
}

int getArrayInt(duk_context *ctx,duk_idx_t indx,int arridx)
{
  int result = 0;
  if(duk_has_prop_index(ctx,indx,arridx))
  {
	  duk_get_prop_index(ctx,-1,arridx);
	  result = duk_to_int(ctx, -1);
      duk_pop(ctx);
  }else
  {
	  result = 2147483647;
  }
  
  return result;
}
const char * getArrayString(duk_context *ctx,duk_idx_t indx,int arridx,char *value)
{  
  const char *result = value;
  if(duk_has_prop_index(ctx,-1,arridx))
  {
	  duk_get_prop_index(ctx,-1,arridx);
	  strcpy(value,duk_to_string(ctx, -1));
      duk_pop(ctx);
  }else
  {
	  result = NULL;
  }
  
  return result;
}

duk_ret_t unloadJsonArrayObjArray(duk_context *ctx,duk_idx_t arridx)
{
	if(arridx != DUK_INVALID_INDEX)
	{
		duk_remove(ctx,arridx);
		duk_remove(ctx,arridx - 1);
	}
	return 1;
}

duk_ret_t unloadJsonArray(duk_context *ctx,duk_idx_t arridx)
{
	if(arridx != DUK_INVALID_INDEX)
	{
		duk_remove(ctx,arridx);
		duk_remove(ctx,arridx - 1);
		duk_remove(ctx,arridx - 2);
	}
	return 1;
}

#endif

#ifdef TEST_SET
duk_ret_t delInt(duk_context *ctx,duk_idx_t indx,const char *key) {
  duk_bool_t result;

  duk_push_string(ctx, key);
  result = duk_del_prop(ctx, indx);
  printf("result - %d\n", (int)(result));
  return result;
}

duk_ret_t putInt(duk_context *ctx,duk_idx_t indx,const char *key, const int value) {
  duk_bool_t result;

  duk_push_int(ctx, value);
  result = duk_put_prop_string(ctx, indx, key);
  return result;
}

duk_ret_t putDouble(duk_context *ctx,duk_idx_t indx,const char *key, const double value)
{
	duk_bool_t result;
	char tmp[50];
	memset(tmp,0,50);
	sprintf(tmp,"%.1f",value);
	duk_push_string(ctx, tmp);
	duk_to_number(ctx, -1);
	result = duk_put_prop_string(ctx, indx, key);
	return result;
}

duk_ret_t putString(duk_context *ctx,duk_idx_t indx,const char *key, const char *value) {
  duk_bool_t result;

  duk_push_string(ctx, value);
  result = duk_put_prop_string(ctx, indx, key);
  return result;
}

duk_ret_t putIntToString(duk_context *ctx,duk_idx_t indx,const char *key, const int value)
{
  duk_bool_t result;
  char tmp[50];
  memset(tmp,0,50);
  sprintf(tmp,"%d",value);
  duk_push_string(ctx, tmp);
  result = duk_put_prop_string(ctx, indx, key);
  return result;
}

duk_ret_t putUIntToString(duk_context *ctx,duk_idx_t indx,const char *key, const unsigned int value)
{
	duk_bool_t result;
  char tmp[50];
  memset(tmp,0,50);
  sprintf(tmp,"%u",value);
  duk_push_string(ctx, tmp);
  result = duk_put_prop_string(ctx, indx, key);
  return result;
}

duk_ret_t putDoubleToString(duk_context *ctx,duk_idx_t indx,const char *key, const double value)
{
  duk_bool_t result;
  char tmp[50];
  memset(tmp,0,50);
  sprintf(tmp,"%.1f",value);
  duk_push_string(ctx, tmp);
  result = duk_put_prop_string(ctx, indx, key);
  return result;
}

duk_ret_t putObjProp(duk_context *ctx,duk_idx_t indx,const char *obj)
{
	duk_bool_t result;
	
	duk_push_object(ctx);
	result = duk_put_prop_string(ctx, indx, obj);
    return result;
}

duk_ret_t putArrayProp(duk_context *ctx,duk_idx_t indx,const char *obj)
{
	duk_bool_t result;
	
	duk_push_array(ctx);
	result = duk_put_prop_string(ctx, indx, obj);
    return result;
}

duk_ret_t putArrayIndexObj(duk_context *ctx,duk_idx_t indx,int idx,duk_idx_t *array_obj)
{
	duk_push_object(ctx);
	duk_put_prop_index(ctx,indx,idx);
	duk_get_prop_index(ctx,indx,idx);
	*array_obj = duk_get_top_index(ctx);
	return 1;
}

duk_ret_t putArrayInt(duk_context *ctx,duk_idx_t indx,const char *obj,int arridx,const char *key, const int value)
{
  duk_bool_t result;
  duk_get_prop_string(ctx,indx,obj);
  if(duk_has_prop_index(ctx,-1,arridx))
  {
	  duk_get_prop_index(ctx,-1,arridx);
	  duk_push_int(ctx, value);
	  result = duk_put_prop_string(ctx, -2, key);
      duk_pop_2(ctx);
  }else
  {
	  duk_push_object(ctx);
	  duk_push_int(ctx, value);
	  result = duk_put_prop_string(ctx, -2, key);
	  duk_put_prop_index(ctx,-2,arridx);
	  duk_pop(ctx);
  }
  
  return result;
}

duk_ret_t putArrayInt2(duk_context *ctx,duk_idx_t indx,const char *obj,int arridx,const int value)
{
  duk_bool_t result;
  duk_get_prop_string(ctx,indx,obj);
  duk_push_int(ctx, value);
  result = duk_put_prop_index(ctx, -2, arridx);
  duk_pop(ctx);
  return result;
}

duk_ret_t putArrayString(duk_context *ctx,duk_idx_t indx,const char *obj,int arridx,const char *key, const char *value)
{  
  duk_bool_t result;
  duk_get_prop_string(ctx,indx,obj);
  if(duk_has_prop_index(ctx,-1,arridx))
  {
	  duk_get_prop_index(ctx,-1,arridx);
	  duk_push_string(ctx, value);
	  result = duk_put_prop_string(ctx, -2, key);
      duk_pop_2(ctx);
  }else
  {
	  duk_push_object(ctx);
	  duk_push_string(ctx, value);
	  result = duk_put_prop_string(ctx, -2, key);
	  duk_put_prop_index(ctx,-2,arridx);
	  duk_pop(ctx);
  }
  
  return result;
}


duk_idx_t putObj(duk_context *ctx)
{
	return duk_push_object(ctx);
}

duk_ret_t putObjInt(duk_context *ctx,duk_idx_t indx,const char *obj,const char *key, const int value)
{
  duk_bool_t result;
  duk_get_prop_string(ctx,indx,obj);
  duk_push_int(ctx, value);
  result = duk_put_prop_string(ctx, -2, key);
  duk_pop(ctx);
  return result;
}

duk_ret_t putObjDouble(duk_context *ctx,duk_idx_t indx,const char *obj,const char *key, const double value)
{
  duk_bool_t result;
  duk_get_prop_string(ctx,indx,obj);
  duk_push_number(ctx, value);
  result = duk_put_prop_string(ctx, -2, key);
  duk_pop(ctx);
  return result;
}

duk_ret_t putObjULLONG(duk_context *ctx,duk_idx_t indx,const char *obj,const char *key, const unsigned long long value)
{
  duk_bool_t result;
  duk_get_prop_string(ctx,indx,obj);
  duk_push_number(ctx, value);
  result = duk_put_prop_string(ctx, -2, key);
  duk_pop(ctx);
  return result;
}

duk_ret_t putObjString(duk_context *ctx,duk_idx_t indx,const char *obj,const char *key, const char *value)
{
  duk_bool_t result;
  duk_get_prop_string(ctx,indx,obj);
  duk_push_string(ctx, value);
  result = duk_put_prop_string(ctx, -2, key);
  duk_pop(ctx);
  return result;
}

const char *generateResult(duk_context *ctx,duk_idx_t returnParamjson,unsigned long id)
{
	const char *pdata = NULL;
	duk_json_encode(ctx,returnParamjson);
	printf("%s====%s\n",__FUNCTION__,duk_to_string(ctx,returnParamjson));
	pdata = duk_base64_encode(ctx,returnParamjson);
	
	duk_push_object(ctx);
	duk_push_number(ctx,0);
	duk_put_prop_string(ctx, -2, "Result");
	duk_push_number(ctx,id);
	duk_put_prop_string(ctx, -2, "ID");
	duk_push_string(ctx,pdata);
	duk_put_prop_string(ctx, -2, "return_Parameter");
	duk_remove(ctx,returnParamjson);
	return duk_json_encode(ctx,-1);
}

#endif

//返回结果被保存在JS堆栈中，
#ifdef MAP
int mapInitialize() {
  contextMap = duk_create_heap_default();
  if (NULL == contextMap) {
    fprintf(stderr, "duk_create_heap_default() Failed!\n");
    return -1;
  }
  else {
    indexMap = duk_push_object(contextMap);
    printf("indexMap - %d\n", indexMap);
  }
  return 0;
}

int mapDestroy() {
	if(contextMap != NULL)
	{
		duk_pop(contextMap);
		duk_destroy_heap(contextMap);
	}
	return 0;
}

duk_ret_t mapSetInt(const char *key, int value) {
  duk_bool_t result;

  pthread_mutex_lock(&mutexMap);
  duk_push_int(contextMap, value);
  result = duk_put_prop_string(contextMap, indexMap, key);
  pthread_mutex_unlock(&mutexMap);
  
  printf("---------mapSetInt:%d\n",result);
  return result;
}

duk_ret_t mapSetString(const char *key, const char *value) {
  duk_bool_t result;

  pthread_mutex_lock(&mutexMap);
  duk_push_string(contextMap, value);
  result = duk_put_prop_string(contextMap, indexMap, key);
  pthread_mutex_unlock(&mutexMap);
  return result;
}

duk_ret_t mapSet() {
  mapSetString("Name", "CaoXx");
  mapSetInt("Age", 30);
  mapSetString("Sex", "Female");
  mapSetString("Nationality", "Chinese");
//printf("JSON - %s\n", duk_json_encode(contextMap, indexMap));
//duk_json_decode(contextMap, indexMap);
  return 1;
}

duk_ret_t mapDel(const char *key) {
  duk_bool_t result;

  pthread_mutex_lock(&mutexMap);
  duk_push_string(contextMap, key);
  result = duk_del_prop(contextMap, indexMap);
  pthread_mutex_unlock(&mutexMap);
  printf("mapdel result - %d\n", (int)(result));
//printf("JSON - %s\n", duk_json_encode(contextMap, -1));
//duk_json_decode(contextMap, indexMap);
  return 1;
}

int mapGet(const char *key) {
  int result = -1;

  pthread_mutex_lock(&mutexMap);
  duk_push_string(contextMap, key);
  if (duk_has_prop(contextMap, indexMap)) {
   printf("--------mapGet Found!\n");
  //所获取内容被放置到堆栈的下一位置（indexMap - 1）
    duk_get_prop_string(contextMap, indexMap, key);
  //当key 不存在时，返回“0 ”
    result = duk_to_int(contextMap, (indexMap - 1));
    duk_pop(contextMap);
  }
  else {
  printf("--------mapGet No Found!\n");
    result = 2147483647;
  }
  pthread_mutex_unlock(&mutexMap);
  return result;
}
#endif

int main_t(int argc, const char *argv[]) {
	duk_context *ctx = NULL;
	duk_idx_t indx;
  mapInitialize();
  jsonInitialize(&ctx,&indx);

  char str[] = "{\"MacList\":[\"111\",\"222\"]}";
  char str1[] = "{\"Parameter\":{\"MacList\":[\"111\",\"222\"]}}";

  char buf[100];

        //duk_idx_t array_obj = getObj(ctx,indx0, "MacList");
        //printf("array_obj %d\n", array_obj);

        int len;
        duk_ret_t ret ;
        duk_idx_t array_obj;

        ret = loadJsonArray(ctx, str1, "Parameter", "MacList", &len, &array_obj);
        
        //ret = loadJsonArrayEx(ctx, str1, "MacList", &len, &array_obj);

        printf("loadJsonArray ret %d, %d, %d\n", ret, len, array_obj);
        getArrayString(ctx, array_obj, 0, buf);
        printf("array_obj 0 %s\n", buf);
        getArrayString(ctx, array_obj, 1, buf);
        printf("array_obj 1 %s\n", buf);
        return 0;

 #ifdef TEST_GET1
  char tmp[100];
  char tmp1[100];
  char tmp2[100];
 char str[] = "{\"Name\":\"LuanSh\",\"Age\":33,\"Name1\":\"LuanSh\",\"Name2\":\"LuanSh\"}";
 const char *pname = getString(ctxjson,str, "Name",tmp);
 const char *pname1 = getString(ctxjson,str, "Name1",tmp1);
 const char *pname2 = getString(ctxjson,str, "Name2",tmp2);
  int iage = getInt(ctxjson,str, "Age");
  printf("Name - %s\n", pname);
  printf("Name1 - %s\n", pname1);
  printf("Name2 - %s\n", pname2);
  printf("Age - %d\n", iage);
//printf("Name - %s\n", getString(ctxjson,"{\"Name\":\"LuanSh\",\"Age\":33}", "NULL"));
//printf("Age - %d\n", getInt(ctxjson,"{\"Name\":\"LuanSh\",\"Age\":33}", "NULL"));
 #endif

  mapSet();

 #ifdef TEST_SET1
  putString(ctxjson,indx,"Name", "LuanSh");
  putInt(ctxjson,indx,"Age", 33);
  putArrayProp(ctxjson,indx,"xx");
 // delInt(ctxjson,indx,"Name");
//delInt("NULL");
  printf("JSON - %s\n", duk_json_encode(ctxjson, indx));
 #endif

  mapDel("Sex");
  printf("Age - %d\n", mapGet("Age"));

  mapDestroy();
  return 0;
}

