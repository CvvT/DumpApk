//
// Created by 陈伟腾 on 15/8/5.
//

#ifndef DUMPAPK_DUMP_H
#define DUMPAPK_DUMP_H

#include <stdlib.h>
#include <android/log.h>
#include "DexFile.h"
#include <stdio.h>

#ifndef LOG_TAG
#define LOG_TAG "cc"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#endif

struct DvmDex {
    DexFile* pDexFile; //DexFile*
    //...
};

struct JarFile {
    u4* Nocare[9];
    char* cacheFileName;
    DvmDex* pDvmDex;
};

struct RawDexFile {
    char* cacheFileName;
    struct DvmDex* pDvmDex;  //DvmDex*
};

struct DexorJar {
    char* fileName;
    bool isDex;
    bool okayToFress;
    struct RawDexFile* pRawDexFile;  //RawDexFile*
    struct JarFile* pJarFile;     //JarFile*
    char* pDexMemory;   //u1*
};

struct CookieMap {
    int cookie;
    DexFile *dexFile;
    struct CookieMap* next;
};

struct EncodedField{
    u4 field_idx_diff;
    u4 access_flag;
};

struct EncodedMethod{
    u4 method_idx_diff;
    u4 access_flag;
    u4 code_off;
};

struct ClassDataItem{
    u4 static_field_size;
    u4 instance_field_size;
    u4 direct_method_size;
    u4 virtual_method_size;
    EncodedField* static_fields;
    EncodedField* instance_fields;
    EncodedMethod* direct_methods;
    EncodedMethod* virtual_methods;
};

class MyDexFile{
public:
    DexFile* dexFile;
    MyDexFile(DexFile* dex, bool shouldmalloc){
        if(shouldmalloc){
            dexFile = (DexFile*)malloc(sizeof(DexFile));
            dexFile->pHeader = (DexHeader*)malloc(sizeof(DexHeader));
            memcpy((void*)dexFile->pHeader, dex->pHeader, sizeof(DexHeader));
        }else{
            dexFile = dex;
        }
    }
};

class MyClassDef{
public:
    DexClassDef* classDef;
    void* interfaceRef;
    void* annotationRef;
    void* classdataRef;
    void* staticvalueRef;

    MyClassDef(bool shouldmalloc, DexClassDef* defclass){
        if (shouldmalloc){
            classDef = new DexClassDef();
            memcpy(classDef, &defclass, sizeof(DexClassDef));
        }else{
            classDef = defclass;
        }
    }

    void dump(DexFile* dexFile){

    }
};

#endif //DUMPAPK_DUMP_H
