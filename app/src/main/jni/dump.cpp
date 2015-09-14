//
// Created by 陈伟腾 on 15/8/5.
//

#include <jni.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "dump.h"

struct CookieMap *allCookie = NULL;
static const char *dumppath = "/data/data/com.cc.test/";

void saveCookie(JNIEnv *env, jobject obj, jint cookie, jobject loader);

void dump(JNIEnv *env, jobject obj, jint cookie, jobject loader);

unsigned int readunsignedleb(const u1 **data) {
    unsigned int result = 0;
    unsigned int shift = 0;
    while (true) {
        u1 byte = **data;
        (*data)++;
        result |= ((byte & 0x7f) << shift);
        if ((byte & 0x80) == 0)
            break;
        shift += 7;
    }
    return result;
}

int readSignedLeb128(const u1** data){
    int result = 0;
    int shift = 0;
    int size = 32;
    while(true) {
        u1 byte = *(*data)++;
        result |= ((byte & 0x7f) << shift);
        shift += 7;
        if ((byte & 0x80) == 0)
            break;
    }
/* sign bit of byte is second high order bit (0x40) */
    if ((shift <size) && (result & (1 << shift)))
    /* sign extend */
    result |= - (1 << shift);
    return result;
}

void writeLeb128(u1** ptr, u4 data)
{
    while (true) {
        u1 out = (u1)(data & 0x7f);
        data >>= 7;
        if (data) {
            *(*ptr)++ = (u1)(out | 0x80);
        } else {
            *(*ptr)++ = out;
            break;
        }
    }
}

u4 unsignedLeb128Size(u4 data){
    u4 ret = 0;
    while(true){
        u4 out = data & 0x7f;
        ret++;
        if (out != data){
            data >>= 7;
        }else {
            break;
        }
    }
    return ret;
}

static JNINativeMethod gMethods[] = {
        {"saveDexFileByCookie", "(ILjava/lang/ClassLoader;)V", (void *) saveCookie},
        {"dumpDexFileByCookie", "(ILjava/lang/ClassLoader;)V", (void *) dump},
};

/*
* 为某一个类注册本地方法
*/
static int registerNativeMethods(JNIEnv *env, const char *className, JNINativeMethod *gMethods,
                                 int numMethods) {
    jclass clazz;
    clazz = env->FindClass(className);
    if (clazz == NULL) {
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        return JNI_FALSE;
    }

    return JNI_TRUE;
}


/*
* 为所有类注册本地方法
*/
static int registerNatives(JNIEnv *env) {
    const char *kClassName = "com/android/reverse/util/NativeFunction";//指定要注册的类
    return registerNativeMethods(env, kClassName, gMethods,
                                 sizeof(gMethods) / sizeof(gMethods[0]));
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;
    jint result = -1;

    LOGE("in jni onload");
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        return -1;
    }

    LOGE("register natives");
    if (!registerNatives(env)) {//注册
        LOGE("register failed");
        return -1;
    }
    //成功
    result = JNI_VERSION_1_4;
    LOGE("register success");
    return result;
}

void printNear(unsigned int *addr, int len) {
    LOGE("start print addr");
    for (int i = 0; i < len / 4;) {
        LOGE("0x%x 0x%x 0x%x 0x%x", *(addr + i), *(addr + 1 + i), *(addr + 2 + i), *(addr + 3 + i));
        i += 4;
    }
}

void saveHeaderAndData(DexFile *dexfile) {
    char *tmp = new char[100];
    const u1 *addr;
    strcpy(tmp, dumppath);
    strcat(tmp, "header");
    FILE *fp = fopen(tmp, "wb+");
//    u4 length = dexfile->pHeader->classDefsOff + sizeof(DexOptHeader);
//    fwrite(dexfile->pOptHeader, 1, length, fp);
    u4 length = dexfile->pHeader->classDefsOff;
    fwrite(dexfile->baseAddr, 1 , length, fp);
    fflush(fp);
    fclose(fp);

    strcpy(tmp, dumppath);
    strcat(tmp, "data");
    fp = fopen(tmp, "wb+");
    addr = dexfile->baseAddr + dexfile->pHeader->classDefsOff + dexfile->pHeader->classDefsSize*
                                                                sizeof(DexClassDef);
    length = dexfile->pHeader->dataSize + dexfile->pHeader->dataOff - (addr - dexfile->baseAddr);
    fwrite(addr, 1, length, fp);
    fflush(fp);

    fclose(fp);
    delete tmp;
}

void saveCookie(JNIEnv *env, jobject obj, jint cookie, jobject loader) {
    if (cookie == 0)
        return;
    LOGE("cookie: %d", cookie);
    DexorJar *pDexorJar = (DexorJar *) cookie;
    LOGE("filename: %s", pDexorJar->fileName);
    const char *skip = "/data/data/de.robv.android.xposed.installer";
    if (!strncmp(skip, pDexorJar->fileName, strlen(skip)))
        return;
    DexFile *dexfile = NULL;
    if (pDexorJar->isDex) {
//        LOGE("cache filename: %s", pDexorJar->pRawDexFile->cacheFileName);
        LOGE("cache filename : %s", pDexorJar->pJarFile->cacheFileName);
        LOGE("it's a dexfile, addr: %d", (int) (pDexorJar->pRawDexFile->pDvmDex->pDexFile));
        dexfile = pDexorJar->pRawDexFile->pDvmDex->pDexFile;
    } else {
        LOGE("cache filename : %s", pDexorJar->pJarFile->cacheFileName);
        LOGE("it's a jarfile, addr: 0x%x", (int) (pDexorJar->pJarFile->pDvmDex->pDexFile));
        dexfile = pDexorJar->pJarFile->pDvmDex->pDexFile;
    }
    if (memcmp(dexfile->pOptHeader->magic, DEX_OPT_MAGIC, 4) == 0) {
        if (memcmp(dexfile->pOptHeader->magic + 4, DEX_OPT_MAGIC_VERS, 4) != 0) {
            LOGE("bad opt version");
            goto bad;
        }
        if (allCookie == NULL) {
            allCookie = (struct CookieMap *) malloc(sizeof(CookieMap));
            if (allCookie == 0)
                goto bad;
            allCookie->cookie = (int) cookie;
            allCookie->dexFile = dexfile;
            allCookie->next = NULL;
        } else {
            struct CookieMap *next = allCookie;
            if (next->cookie == cookie)
                goto bad;
            while (next->next != NULL) {
                if (next->cookie == cookie)
                    goto bad;
                next = next->next;
            }
            next->next = new CookieMap();
            next = next->next;
            next->cookie = (int) cookie;
            next->dexFile = dexfile;
            next->next = NULL;
        }
    }
//    if (strstr(pDexorJar->fileName, "libmobisec"))
//        dump(env, obj, cookie, loader);
    bad:
    return;
}

struct CookieMap *findcookie(int cookie) {
    struct CookieMap *next = allCookie;
    while (next != NULL) {
        if (next->cookie == cookie)
            break;
        LOGE("next cookie=%d", next->cookie);
        next = next->next;
    }
    if (next == NULL) {
        LOGE("unsaved cookie");
        goto bad;
    } else {
        LOGE("find cookie!");
    }
    return next;
    bad:
    return NULL;
}

JNIEXPORT void JNICALL copyinfo(JNIEnv *env, jobject, jint cookie) {
    struct CookieMap *dexfile = findcookie(cookie);
    if (dexfile == NULL)
        goto bad;

    bad:;
}

ClassDataItem *readClassData(const u1 **data) {
    if (data == NULL)
        return NULL;
    ClassDataItem *ret = (ClassDataItem *) malloc(sizeof(ClassDataItem));
    if (ret == NULL)
        return NULL;
    ret->static_field_size = readunsignedleb(data);
    LOGE("static field size=%d", ret->static_field_size);
    ret->instance_field_size = readunsignedleb(data);
    LOGE("instance field size=%d", ret->instance_field_size);
    ret->direct_method_size = readunsignedleb(data);
    LOGE("direct method size=%d", ret->direct_method_size);
    ret->virtual_method_size = readunsignedleb(data);
    LOGE("virtual method size=%d", ret->virtual_method_size);
    if (ret->static_field_size)
        ret->static_fields = (EncodedField *) malloc(sizeof(EncodedField) * ret->static_field_size);
    else ret->static_fields = NULL;
    if (ret->instance_field_size)
        ret->instance_fields = (EncodedField *) malloc(
                sizeof(EncodedField) * ret->instance_field_size);
    else ret->instance_fields = NULL;
    if (ret->direct_method_size)
        ret->direct_methods = (EncodedMethod *) malloc(
                sizeof(EncodedMethod) * ret->direct_method_size);
    else ret->direct_methods = NULL;
    if (ret->virtual_method_size)
        ret->virtual_methods = (EncodedMethod *) malloc(
                sizeof(EncodedMethod) * ret->virtual_method_size);
    else ret->virtual_methods = NULL;
    for (u4 i = 0; i < ret->static_field_size; i++) {
        ret->static_fields[i].field_idx_diff = readunsignedleb(data);
        ret->static_fields[i].access_flag = readunsignedleb(data);
    }
    for (u4 i = 0; i < ret->instance_field_size; i++) {
        ret->instance_fields[i].field_idx_diff = readunsignedleb(data);
        ret->instance_fields[i].access_flag = readunsignedleb(data);
    }
    for (u4 i = 0; i < ret->direct_method_size; i++) {
        ret->direct_methods[i].method_idx_diff = readunsignedleb(data);
        ret->direct_methods[i].access_flag = readunsignedleb(data);
        ret->direct_methods[i].code_off = readunsignedleb(data);
    }
    for (u4 i = 0; i < ret->virtual_method_size; i++) {
        ret->virtual_methods[i].method_idx_diff = readunsignedleb(data);
        ret->virtual_methods[i].access_flag = readunsignedleb(data);
        ret->virtual_methods[i].code_off = readunsignedleb(data);
    }
    return ret;
}

void slashtodot(char *dest, const char *source) {
    strcpy(dest, source + 1);
    dest[strlen(dest) - 1] = '\0';
    for (u1 j = 0; j < strlen(dest); j++)
        if (dest[j] == '/')
            dest[j] = '.';
}

u1* EncodeClassData(ClassDataItem *pData, u4& len)
{
    len=0;

    len+=unsignedLeb128Size(pData->static_field_size);
    len+=unsignedLeb128Size(pData->instance_field_size);
    len+=unsignedLeb128Size(pData->direct_method_size);
    len+=unsignedLeb128Size(pData->virtual_method_size);

    if (pData->static_field_size) {
        for (uint32_t i = 0; i < pData->static_field_size; i++) {
            len+=unsignedLeb128Size(pData->static_fields[i].field_idx_diff);
            len+=unsignedLeb128Size(pData->static_fields[i].access_flag);
        }
    }

    if (pData->instance_field_size) {
        for (uint32_t i = 0; i < pData->instance_field_size; i++) {
            len+=unsignedLeb128Size(pData->instance_fields[i].field_idx_diff);
            len+=unsignedLeb128Size(pData->instance_fields[i].access_flag);
        }
    }

    if (pData->direct_method_size) {
        for (uint32_t i=0; i<pData->direct_method_size; i++) {
            len+=unsignedLeb128Size(pData->direct_methods[i].method_idx_diff);
            len+=unsignedLeb128Size(pData->direct_methods[i].access_flag);
            len+=unsignedLeb128Size(pData->direct_methods[i].code_off);
        }
    }

    if (pData->virtual_method_size) {
        for (uint32_t i=0; i<pData->virtual_method_size; i++) {
            len+=unsignedLeb128Size(pData->virtual_methods[i].method_idx_diff);
            len+=unsignedLeb128Size(pData->virtual_methods[i].access_flag);
            len+=unsignedLeb128Size(pData->virtual_methods[i].code_off);
        }
    }

    u1* store = (u1*)malloc(len);

    if (!store) {
        return NULL;
    }

    u1* result=store;

    writeLeb128(&store,pData->static_field_size);
    writeLeb128(&store,pData->instance_field_size);
    writeLeb128(&store,pData->direct_method_size);
    writeLeb128(&store,pData->virtual_method_size);

    if (pData->static_field_size) {
        for (uint32_t i = 0; i < pData->static_field_size; i++) {
            writeLeb128(&store,pData->static_fields[i].field_idx_diff);
            writeLeb128(&store,pData->static_fields[i].access_flag);
        }
    }

    if (pData->instance_field_size) {
        for (uint32_t i = 0; i < pData->instance_field_size; i++) {
            writeLeb128(&store,pData->instance_fields[i].field_idx_diff);
            writeLeb128(&store,pData->instance_fields[i].access_flag);
        }
    }

    if (pData->direct_method_size) {
        for (uint32_t i=0; i<pData->direct_method_size; i++) {
            writeLeb128(&store,pData->direct_methods[i].method_idx_diff);
            writeLeb128(&store,pData->direct_methods[i].access_flag);
            writeLeb128(&store,pData->direct_methods[i].code_off);
        }
    }

    if (pData->virtual_method_size) {
        for (uint32_t i=0; i<pData->virtual_method_size; i++) {
            writeLeb128(&store,pData->virtual_methods[i].method_idx_diff);
            writeLeb128(&store,pData->virtual_methods[i].access_flag);
            writeLeb128(&store,pData->virtual_methods[i].code_off);
        }
    }
    return result;
}

u1* codeitem_end(const u1** pData)
{
    uint32_t num_of_list = readunsignedleb(pData);
    for (;num_of_list>0;num_of_list--) {
        int32_t num_of_handlers=readSignedLeb128(pData);
        int num=num_of_handlers;
        if (num_of_handlers<=0) {
            num=-num_of_handlers;
        }
        for (; num > 0; num--) {
            readunsignedleb(pData);
            readunsignedleb(pData);
        }
        if (num_of_handlers<=0) {
            readunsignedleb(pData);
        }
    }
    return (u1*)(*pData);
}

void getProtoString(char* proto_str, DexFile* dex, const DexMethodId* methodid){
    const char *tmp_str;
    char count = 0;
    const DexProtoId *protoid = dexGetProtoId(dex, methodid->protoIdx);
    const DexTypeList *params = dexGetProtoParameters(dex, protoid);
    strncpy(proto_str, "(", 1);
    count++;
    if (params != NULL) {
        for (u4 l = 0; l < params->size; l++) {
            tmp_str = dexStringByTypeIdx(dex, params->list[l].typeIdx);
            strcpy(proto_str + count, tmp_str);
            count += strlen(tmp_str);
        }
    }
    strncpy(proto_str + count, ")", 1);
    count++;
    tmp_str = dexStringByTypeIdx(dex, protoid->returnTypeIdx);
    strcpy(proto_str + count, tmp_str);
    count += strlen(tmp_str);
    proto_str[count] = '\0';
}

void dump(JNIEnv *env, jobject obj, jint cookie, jobject loader) {
    struct CookieMap *dexfile = findcookie(cookie);
    DexFile *dex = NULL;
    const char *header = "Landroid";
//    const char *target = "Lcom/cc/test/MainActivity;";
    u4 pre = 0, pointer = 0, start = 0, end = 0;
    jmethodID clinit, findclassMethod;
    jclass classLoaderClass;
    char padding = 0;

    if (dexfile == NULL)
        return;
    dex = dexfile->dexFile;
    pointer = dex->pHeader->dataOff + dex->pHeader->dataSize;
    start = dex->pHeader->dataOff;
    end = pointer;

    char *path = new char[100];
    strcpy(path, dumppath);
    strcat(path, "classdef");
    FILE *fp = fopen(path, "wb+");

    strcpy(path, dumppath);
    strcat(path, "extra");
    FILE *fp1 = fopen(path, "wb+");

    classLoaderClass = env->FindClass("java/lang/ClassLoader");
    if (classLoaderClass == NULL) {
        LOGE("error: didn't find class-java/lang/ClassLoader");
        return;
    } else
        LOGE("get java/lang/ClassLoader");
    findclassMethod = env->GetMethodID(classLoaderClass, "loadClass",
                                       "(Ljava/lang/String;)Ljava/lang/Class;");
    if (findclassMethod == NULL) {
        LOGE("error: didn't get method-loadClass");
        return;
    } else
        LOGE("get method loadClass");

    int page_size = sysconf(_SC_PAGESIZE);
    u4 npage = dex->pHeader->fileSize / page_size + ((dex->pHeader->fileSize % page_size == 0) ? 0 : 1);
    LOGE("npage=%d, page_size=%d", npage, page_size);
    if(mprotect((void *) ((int)(dex->baseAddr) / page_size * page_size), npage*page_size, PROT_READ | PROT_EXEC | PROT_WRITE) != 0){
        LOGE("mem privilege change failed");
        return;
    }

    for (unsigned int i = 0; i < dex->pHeader->classDefsSize; i++) {
        const DexClassDef *classDef = dexGetClassDef(dex, i);
        const char *descriptor = dexGetClassDescriptor(dex, classDef);
        bool need_extra = false, pass = false;
        char *tmp;
        jclass clazz;
        const u1 *data;
        ClassDataItem *pdata = NULL;

        if (!strncmp(header, descriptor, 8) || !classDef->classDataOff) {
            pass = true;
            goto classdef;
        }

        tmp = (char *) malloc(strlen(descriptor) - 1);
        slashtodot(tmp, descriptor);
        LOGE("tmp descriptor is=%s", tmp);
        clazz = (jclass) env->CallObjectMethod(loader, findclassMethod,
                                                      env->NewStringUTF(tmp));
        if (env->ExceptionOccurred()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        if (!clazz) {
            LOGE("error: didn't find class");
            continue;
        } else
            LOGE("find the class");

        clinit = (*env).GetStaticMethodID(clazz, "<clinit>", "()V");
        if (!clinit) {
            LOGE("error : class %s dont has <clinit> method", tmp);
            env->ExceptionClear();
        } else {
            LOGE("get method <clinit>");
            env->CallStaticVoidMethod(clazz, clinit);
        }

        if (classDef->classDataOff < start || classDef->classDataOff > end){
            need_extra = true;
        }

        data = dexGetClassData(dex, classDef);
        pdata = readClassData(&data);
        if (pdata == NULL) {
            LOGE("error: null ptr");
            continue;
        }

        if (pdata->direct_method_size) {
            pre = 0;
            for (u4 index = 0; index < pdata->direct_method_size; index++) {
                pre += pdata->direct_methods[index].method_idx_diff;
                const DexMethodId *methodid = dexGetMethodId(dex, pre);
                const char *name = dexStringById(dex, methodid->nameIdx);
                char proto_str[256];
                jmethodID method;
                getProtoString(proto_str, dex, methodid);

                LOGE("name=%s, sig=%s", name, proto_str);
                method = env->GetMethodID(clazz, name, proto_str);
                if (method == NULL) {
                    LOGE("it's not a Non-Static Method, try CallStaticMethodID");
                    env->ExceptionClear();
                    method = env->GetStaticMethodID(clazz, name, proto_str);
                }
                if (method) {
//                    LOGE("Get Method, access flag=%x", *(u4 * )((u4) method + 4));
                    u4 access = *(u4* )((u4)method + 4);
                    u4 codeoff = *(u4 *)((u4)method + 32);
                    u4 nativeFunc = *(u4 *)((u4)method + 40);
                    if (!codeoff || access & 0x100) {
                        if (pdata->direct_methods[index].code_off){ //in order to fix it
                            need_extra = true;
                            pdata->direct_methods[index].access_flag = access;
                            pdata->direct_methods[index].code_off = 0;
                        }
                        continue;
                    }

                    u4 codeitem_off = codeoff - 16 - (u4)dex->baseAddr;
                    DexCode *code = (DexCode*)(codeoff-16);
                    code->debugInfoOff = 0;
                    if (access != pdata->direct_methods[index].access_flag){
                        need_extra = true;
                        pdata->direct_methods[index].access_flag = access;
                    }
                    if (codeitem_off != pdata->direct_methods[index].code_off
                            &&((codeitem_off>=start&&codeitem_off<=end)||codeitem_off==0)){
                        need_extra = true;
                        pdata->direct_methods[index].code_off = codeitem_off;
//                        DexCode *code = (DexCode*)(codeoff-16);
//                        code->debugInfoOff = 0;
                    }
                    if ((codeitem_off<start || codeitem_off>end) && codeitem_off!=0) {
                        need_extra = true;
                        pdata->direct_methods[index].code_off = pointer;

                        u1 *item=(u1 *) code;
                        u4 code_item_len = 0;
                        if (code->triesSize) {
                            const u1 * handler_data = dexGetCatchHandlerData(code);
                            const u1** phandler= &handler_data;
                            u1 * tail=codeitem_end(phandler);
                            code_item_len = (u4)(tail-item);
                        }else{
                            code_item_len = 16+code->insnsSize*2;
                        }

                        LOGE("the debug is=%x", code->debugInfoOff);
                        fwrite(item,1,code_item_len,fp1);
                        fflush(fp1);
                        pointer += code_item_len;
                        while (pointer & 3) {
                            fwrite(&padding,1,1,fp1);
                            fflush(fp1);
                            pointer++;
                        }
                    }
                } else {
                    LOGE("Failed Get Method");
                }
            }
        }
        if (pdata->virtual_method_size){
            pre = 0;
            for (u4 index = 0; index < pdata->virtual_method_size; index++){
                pre += pdata->virtual_methods[index].method_idx_diff;
                const DexMethodId *methodid = dexGetMethodId(dex, pre);
                const char *name = dexStringById(dex, methodid->nameIdx);
                char proto_str[256];
                jmethodID method;
                getProtoString(proto_str, dex, methodid);

                LOGE("name=%s, sig=%s", name, proto_str);
                method = env->GetMethodID(clazz, name, proto_str);
                if (method == NULL) {
                    LOGE("it's not a Non-Static Method, try CallStaticMethodID");
                    env->ExceptionClear();
                    method = env->GetStaticMethodID(clazz, name, proto_str);
                }
                if (method){
//                    LOGE("Get Method, access flag=%x", *(u4 * )((u4) method + 4));
                    u4 access = *(u4* )((u4)method + 4);
                    u4 codeoff = *(u4 *)((u4)method + 32);
                    u4 nativeFunc = *(u4 *)((u4)method + 40);
                    if (!codeoff || access & 0x100) {
                        if (pdata->virtual_methods[index].code_off){ //in order to fix it
                            need_extra = true;
                            pdata->virtual_methods[index].access_flag = access;
                            pdata->virtual_methods[index].code_off = 0;
                        }
                        continue;
                    }

                    u4 codeitem_off = codeoff - 16 - (u4)dex->baseAddr;
//                    LOGE("start fix, addr=%x", codeoff-16);
                    DexCode *code = (DexCode*)(codeoff-16);
                    code->debugInfoOff = 0;
//                    LOGE("I know you will down here");
                    if (access != pdata->virtual_methods[index].access_flag){
                        need_extra = true;
                        pdata->virtual_methods[index].access_flag = access;
                    }
                    if (codeitem_off != pdata->virtual_methods[index].code_off
                        &&((codeitem_off>=start&&codeitem_off<=end)||codeitem_off==0)){
                        need_extra = true;
                        pdata->virtual_methods[index].code_off = codeitem_off;
                    }
                    if ((codeitem_off<start || codeitem_off>end) && codeitem_off!=0) {
                        need_extra = true;
                        pdata->virtual_methods[index].code_off = pointer;
//                        DexCode *code = (DexCode*)(codeoff-16);
                        // debug
//                        code->debugInfoOff = 0;
                        u1 *item=(u1 *) code;
                        u4 code_item_len = 0;
                        if (code->triesSize) {
                            const u1 * handler_data = dexGetCatchHandlerData(code);
                            const u1** phandler= &handler_data;
                            u1 * tail=codeitem_end(phandler);
                            code_item_len = (u4)(tail-item);
                        }else{
                            code_item_len = 16+code->insnsSize*2;
                        }

                        LOGE("the debug is=%x", code->debugInfoOff);
                        fwrite(item,1,code_item_len,fp1);
                        fflush(fp1);
                        pointer += code_item_len;
                        while (pointer & 3) {
                            fwrite(&padding,1,1,fp1);
                            fflush(fp1);
                            pointer++;
                        }
                    }
                }
            }
        }
    classdef:
        DexClassDef temp = *classDef;
        u4* p = (u4*)&temp;
        if (need_extra){
            u4 class_data_len = 0;
            u1 *out = EncodeClassData(pdata,class_data_len);
            if (!out)
                continue;
            temp.classDataOff = pointer;
            fwrite(out, 1, class_data_len, fp1);
            fflush(fp1);
            pointer += class_data_len;
            while (pointer & 3){
                fwrite(&padding, 1, 1, fp1);
                fflush(fp1);
                pointer++;
            }
            free(out);
        }
        if (pdata)
            free(pdata);

//        if (pass){
//            temp.classDataOff = 0;
//            temp.annotationsOff = 0;
//        }
        fwrite(p, sizeof(DexClassDef), 1, fp);
        fflush(fp);
    }

    //dump optdex
//    fwrite(dex->baseAddr - sizeof(DexOptHeader) + dex->pOptHeader->depsOffset, 1,
//           dex->pOptHeader->optOffset - dex->pOptHeader->depsOffset + dex->pOptHeader->optLength, fp1);
//    LOGE("%d, %d, %d", dex->pOptHeader->optOffset, dex->pOptHeader->depsOffset, dex->pOptHeader->optLength);
//    fflush(fp1);
    fclose(fp1);
    fclose(fp);

//    LOGE("start fix optHeader");
//    DexOptHeader* head = (DexOptHeader*)dex->pOptHeader;
//    head->optOffset = pointer + (head->optOffset - head->depsOffset);
//    head->depsOffset = pointer;
//    LOGE("end fix optHeader, %x, %x", pointer, head->optOffset);
    saveHeaderAndData(dex);
    strcpy(path, dumppath);
    strcat(path, "whole.dex");
    fp = fopen(path, "wb+");
    rewind(fp);

    int fd = -1;
    int r = -1;
    int len = 0;
    char *addr = NULL;
    struct stat st;

    strcpy(path, dumppath);
    strcat(path, "header");

    fd = open(path,O_RDONLY,0666);
    if (fd == -1) {
        return;
    }

    r=fstat(fd,&st);
    if(r==-1){
        close(fd);
        return;
    }

    len=st.st_size;
    addr=(char*)mmap(NULL,len,PROT_READ,MAP_PRIVATE,fd,0);
    fwrite(addr,1,len,fp);
    fflush(fp);
    munmap(addr,len);
    close(fd);

    strcpy(path,dumppath);
    strcat(path,"classdef");

    fd=open(path,O_RDONLY,0666);
    if (fd==-1) {
        return;
    }

    r=fstat(fd,&st);
    if(r==-1){
        close(fd);
        return;
    }

    len=st.st_size;
    addr=(char*)mmap(NULL,len,PROT_READ,MAP_PRIVATE,fd,0);
    fwrite(addr,1,len,fp);
    fflush(fp);
    munmap(addr,len);
    close(fd);

    strcpy(path,dumppath);
    strcat(path,"data");

    fd=open(path,O_RDONLY,0666);
    if (fd==-1) {
        return;
    }

    r=fstat(fd,&st);
    if(r==-1){
        close(fd);
        return;
    }

    len=st.st_size;
    addr=(char*)mmap(NULL,len,PROT_READ,MAP_PRIVATE,fd,0);
    fwrite(addr,1,len,fp);
    fflush(fp);
    munmap(addr,len);
    close(fd);

//    while (inc>0) {
//        fwrite(&padding,1,1,fp);
//        fflush(fp);
//        inc--;
//    }

    strcpy(path,dumppath);
    strcat(path,"extra");

    fd=open(path,O_RDONLY,0666);
    if (fd==-1) {
        return;
    }

    r=fstat(fd,&st);
    if(r==-1){
        close(fd);
        return;
    }

    len=st.st_size;
    addr=(char*)mmap(NULL,len,PROT_READ,MAP_PRIVATE,fd,0);
    fwrite(addr,1,len,fp);
    fflush(fp);
    munmap(addr,len);
    close(fd);

    fclose(fp);
    delete path;
    bad:
    LOGE("DONE!");
    return;
}


