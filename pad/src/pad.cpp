/**
 * Functionality to work with Play Asset Delivery and the AssetPackManager
 * @namespace pad
 * @language lua
 */

#define LIB_NAME "PAD"
#define MODULE_NAME "pad"

#include <dmsdk/sdk.h>

#if defined(DM_PLATFORM_ANDROID)

#include <dmsdk/dlib/android.h>



struct PADJNI
{
    jobject        m_JNI;
    jmethodID      m_Cancel;
    jmethodID      m_Fetch;
    jmethodID      m_HasPackState;
    jmethodID      m_GetPackState;
    jmethodID      m_RemovePack;
    jmethodID      m_ShowConfirmationDialog;

    jmethodID      m_GetNextEvent;

    jmethodID      m_GetPackLocation;
    jmethodID      m_GetPackBytesDownloaded;
    jmethodID      m_GetPackErrorCode;
    jmethodID      m_GetPackStatus;
    jmethodID      m_GetPackTotalBytesToDownload;
    jmethodID      m_GetPackTransferProgressPercentage;

};

static PADJNI g_PAD;
static dmScript::LuaCallbackInfo* g_PADListener;


static void CallVoidMethod(jobject instance, jmethodID method)
{
    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();
    env->CallVoidMethod(instance, method);
}

static void CallVoidMethodLong(jobject instance, jmethodID method, const uint64_t arg1)
{
    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();
    env->CallVoidMethod(instance, method, arg1);
}

static int CallIntMethodVoid(jobject instance, jmethodID method)
{
    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();
    jint result = env->CallIntMethod(instance, method);
    return result;
}

static float CallFloatMethodInt(jobject instance, jmethodID method, const int arg1)
{
    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();
    jfloat result = env->CallFloatMethod(instance, method, arg1);
    return result;
}


static int CallIntMethodChar(jobject instance, jmethodID method, const char* cstr)
{
    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();

    jstring jstr = env->NewStringUTF(cstr);
    jint result = env->CallIntMethod(instance, method, jstr);
    env->DeleteLocalRef(jstr);
    return result;
}

static long CallLongMethodChar(jobject instance, jmethodID method, const char* cstr)
{
    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();

    jstring jstr = env->NewStringUTF(cstr);
    jlong result = env->CallLongMethod(instance, method, jstr);
    env->DeleteLocalRef(jstr);
    return result;
}


static void CallVoidMethodChar(jobject instance, jmethodID method, const char* cstr)
{
    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();

    jstring jstr = env->NewStringUTF(cstr);
    env->CallVoidMethod(instance, method, jstr);
    env->DeleteLocalRef(jstr);
}

static char* CallCharMethodChar(jobject instance, jmethodID method, const char* cstr)
{
    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();

    jstring jstr = env->NewStringUTF(cstr);
    jstring return_value = (jstring)env->CallObjectMethod(instance, method, jstr);
    env->DeleteLocalRef(jstr);
    if (!return_value)
    {
        return 0;
    }

    const char* result_cstr = env->GetStringUTFChars(return_value, 0);
    char* result_cstr_copy = strdup(result_cstr);
    env->ReleaseStringUTFChars(return_value, result_cstr);
    env->DeleteLocalRef(return_value);

    return result_cstr_copy;
}

static char* CallCharMethodVoid(jobject instance, jmethodID method)
{
    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();

    jstring return_value = (jstring)env->CallObjectMethod(instance, method);
    if (!return_value)
    {
        return 0;
    }

    const char* result_cstr = env->GetStringUTFChars(return_value, 0);
    char* result_cstr_copy = strdup(result_cstr);
    env->ReleaseStringUTFChars(return_value, result_cstr);
    env->DeleteLocalRef(return_value);

    return result_cstr_copy;
}

static void HandleEvent(const char* event)
{
    if (!g_PADListener)
    {
        dmLogWarning("PAD callback is not set");
        return;
    }

    if (!dmScript::IsCallbackValid(g_PADListener))
    {
        dmLogError("PAD callback is not valid");
        g_PADListener = 0;
        return;
    }

    if (!dmScript::SetupCallback(g_PADListener))
    {
        dmLogError("PAD callback setup failed");
        dmScript::DestroyCallback(g_PADListener);
        g_PADListener = 0;
        return;
    }

    lua_State* L = dmScript::GetCallbackLuaContext(g_PADListener);

    dmScript::JsonToLua(L, event, strlen(event));

    // self, event
    int ret = lua_pcall(L, 2, 0, 0);
    if (ret != 0)
    {
        dmLogError("Error invoking PAD listener %d (LUA_ERRRUN = %d LUA_ERRMEM = %d LUA_ERRERR = %d)", ret, LUA_ERRRUN, LUA_ERRMEM, LUA_ERRERR);
        lua_pop(L, 1);
    }
    dmScript::TeardownCallback(g_PADListener);
}

/**
 * Requests to cancel the download of the specified asset packs.
 * @name cancel
 * @string pack_name
 */
static int PADCancel(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    const char* pack_name = luaL_checkstring(L, 1);
    CallVoidMethodChar(g_PAD.m_JNI, g_PAD.m_Cancel, pack_name);
    return 0;
}

/**
 * Requests to download the specified asset pack.
 * @name fetch
 * @string pack_name
 */
static int PADFetch(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    const char* pack_name = luaL_checkstring(L, 1);
    CallVoidMethodChar(g_PAD.m_JNI, g_PAD.m_Fetch, pack_name);
    return 0;
}

/**
 * Deletes the specified asset pack from the internal storage of the app.
 * @name remove_pack
 * @string pack_name
 */
static int PADRemovePack(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    const char* pack_name = luaL_checkstring(L, 1);
    CallVoidMethodChar(g_PAD.m_JNI, g_PAD.m_RemovePack, pack_name);
    return 0;
}

/**
 * Requests download state and details for the specified asset pack. This is an
 * asynchronous function and will send a `PACK_STATE_UPDATED` event once the new
 * state is available.
 * @name get_pack_state
 * @string pack_name
 */
static int PADGetPackState(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    const char* pack_name = luaL_checkstring(L, 1);
    CallVoidMethodChar(g_PAD.m_JNI, g_PAD.m_GetPackState, pack_name);
    return 0;
}

/**
 * Returns the location of the specified asset pack on the device or an empty
 * string if this pack is not downloaded.
 * @name get_pack_location
 * @string pack_name
 * @treturn string
 */
static int PADGetPackLocation(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    const char* pack_name = luaL_checkstring(L, 1);
    char* location = CallCharMethodChar(g_PAD.m_JNI, g_PAD.m_GetPackLocation, pack_name);
    lua_pushstring(L, location);
    free(location);
    return 1;
}

/**
 * Returns the total number of bytes already downloaded for the pack.
 * Note that you must have called the `get_pack_state()` function first and wait for
 * a `PACK_STATE_UPDATED` event before calling this function.
 * @name get_pack_bytes_downloaded
 * @string pack_name
 * @treturn bytes_downloaded
 */
static int PADGetPackBytesDownloaded(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    const char* pack_name = luaL_checkstring(L, 1);
    long bytes_downloaded = CallLongMethodChar(g_PAD.m_JNI, g_PAD.m_GetPackBytesDownloaded, pack_name);
    lua_pushnumber(L, bytes_downloaded);
    return 1;
}

/**
 * Returns the error code for the pack, if Play has failed to download the pack.
 * Note that you must have called the `get_pack_state()` function first and wait for
 * a `PACK_STATE_UPDATED` event before calling this function.
 * @name get_pack_error_code
 * @string pack_name
 * @treturn error_code
 */
static int PADGetPackErrorCode(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    const char* pack_name = luaL_checkstring(L, 1);
    int error_code = CallIntMethodChar(g_PAD.m_JNI, g_PAD.m_GetPackErrorCode, pack_name);
    lua_pushnumber(L, error_code);
    return 1;
}

/**
 * Returns the download status of the pack.
 * Note that you must have called the `get_pack_state()` function first and wait for
 * a `PACK_STATE_UPDATED` event before calling this function.
 * @name get_pack_status
 * @string pack_name
 * @treturn status
 */
static int PADGetPackStatus(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    const char* pack_name = luaL_checkstring(L, 1);
    int pack_status = CallIntMethodChar(g_PAD.m_JNI, g_PAD.m_GetPackStatus, pack_name);
    lua_pushnumber(L, pack_status);
    return 1;
}

/**
 * Returns the total size of the pack in bytes.
 * Note that you must have called the `get_pack_state()` function first and wait for
 * a `PACK_STATE_UPDATED` event before calling this function.
 * @name get_pack_total_bytes_to_download
 * @string pack_name
 * @treturn bytes_to_download
 */
static int PADGetPackTotalBytesToDownload(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    const char* pack_name = luaL_checkstring(L, 1);
    long bytes_to_download = CallLongMethodChar(g_PAD.m_JNI, g_PAD.m_GetPackTotalBytesToDownload, pack_name);
    lua_pushnumber(L, bytes_to_download);
    return 1;
}

/**
 * Returns the percentage of the asset pack already transferred to the app.
 * Note that you must have called the `get_pack_state()` function first and wait for
 * a `PACK_STATE_UPDATED` event before calling this function.
 * @name get_pack_transfer_progress_percentage
 * @string pack_name
 * @treturn number
 */
static int PADGetPackTransferProgressPercentage(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 1);
    const char* pack_name = luaL_checkstring(L, 1);
    int percentage = CallIntMethodChar(g_PAD.m_JNI, g_PAD.m_GetPackTransferProgressPercentage, pack_name);
    lua_pushnumber(L, percentage);
    return 1;
}

/** Shows a dialog that asks the user for consent to download packs
 * Shows a dialog that asks the user for consent to download packs that are
 * currently in either the AssetPackStatus.REQUIRES_USER_CONFIRMATION state or
 * the AssetPackStatus.WAITING_FOR_WIFI state. Will return result in event
 * listener.
 * @name show_confirmation_dialog
 * @string pack_name
 */
static int PADShowConfirmationDialog(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    const char* pack_name = luaL_checkstring(L, 1);
    CallVoidMethodChar(g_PAD.m_JNI, g_PAD.m_ShowConfirmationDialog, pack_name);
    return 0;
}

/**
 * Set listener to receive events
 * @name set_listener
 * @function listener The function to call (self, event)
 */
static int PADSetListener(lua_State* L)
{
    DM_LUA_STACK_CHECK(L, 0);
    if (g_PADListener)
    {
        dmScript::DestroyCallback(g_PADListener);
        g_PADListener = 0;
    }
    g_PADListener = dmScript::CreateCallback(L, 1);
    return 0;
}

// Functions exposed to Lua
static const luaL_reg Module_methods[] =
{
    {"set_listener", PADSetListener},
    {"cancel", PADCancel},
    {"fetch", PADFetch},
    {"get_pack_state", PADGetPackState},
    {"get_pack_location", PADGetPackLocation},
    {"get_pack_bytes_downloaded", PADGetPackBytesDownloaded},
    {"get_pack_error_code", PADGetPackErrorCode},
    {"get_pack_status", PADGetPackStatus},
    {"get_pack_total_bytes_to_download", PADGetPackTotalBytesToDownload},
    {"get_pack_transfer_progress_percentage", PADGetPackTransferProgressPercentage},
    {"remove_pack", PADRemovePack},
    {"show_confirmation_dialog", PADShowConfirmationDialog},
    {0, 0}
};


static void LuaInit(lua_State* L)
{
    dmLogInfo("LuaInit");
    int top = lua_gettop(L);
    luaL_register(L, MODULE_NAME, Module_methods);

    // repeated in PADJNI.java
    
    #define SETCONSTANT(name, val) \
    lua_pushinteger(L, (lua_Number) val); \
    lua_setfield(L, -2, #name);

    /**
     * @field EVENT_PACK_STATE_UPDATED
     */
    SETCONSTANT(EVENT_PACK_STATE_UPDATED, 0);
    /**
     * @field EVENT_PACK_STATE_ERROR
     */
     SETCONSTANT(EVENT_PACK_STATE_ERROR, 1);
    /**
     * @field EVENT_REMOVE_PACK_COMPLETED
     */
     SETCONSTANT(EVENT_REMOVE_PACK_COMPLETED, 2);
    /**
     * @field EVENT_REMOVE_PACK_CANCELED
     */
     SETCONSTANT(EVENT_REMOVE_PACK_CANCELED, 3);
    /**
     * @field EVENT_REMOVE_PACK_ERROR
     */
     SETCONSTANT(EVENT_REMOVE_PACK_ERROR, 4);
    /**
     * @field EVENT_DIALOG_CONFIRMED
     */
     SETCONSTANT(EVENT_DIALOG_CONFIRMED, 5);
    /**
     * @field EVENT_DIALOG_DECLINED
     */
     SETCONSTANT(EVENT_DIALOG_DECLINED, 6);
    /**
     * @field EVENT_DIALOG_CANCELED
     */
     SETCONSTANT(EVENT_DIALOG_CANCELED, 7);
    /**
     * @field EVENT_DIALOG_ERROR
     */
     SETCONSTANT(EVENT_DIALOG_ERROR, 8);
    /**
     * @field EVENT_LOG
     */
     SETCONSTANT(EVENT_LOG, 9);


    /**
     * @field STATUS_UNKNOWN
     */
    SETCONSTANT(STATUS_UNKNOWN, 0)
    /**
     * @field STATUS_PENDING
     */
    SETCONSTANT(STATUS_PENDING, 1)
    /**
     * @field STATUS_DOWNLOADING
     */
    SETCONSTANT(STATUS_DOWNLOADING, 2)
    /**
     * @field STATUS_TRANSFERRING
     */
    SETCONSTANT(STATUS_TRANSFERRING, 3)
    /**
     * @field STATUS_COMPLETED
     */
    SETCONSTANT(STATUS_COMPLETED, 4)
    /**
     * @field STATUS_FAILED
     */
    SETCONSTANT(STATUS_FAILED, 5)
    /**
     * @field STATUS_CANCELED
     */
    SETCONSTANT(STATUS_CANCELED, 6)
    /**
     * @field STATUS_WAITING_FOR_WIFI
     */
    SETCONSTANT(STATUS_WAITING_FOR_WIFI, 7)
    /**
     * @field STATUS_NOT_INSTALLED
     */
    SETCONSTANT(STATUS_NOT_INSTALLED, 8)
    /**
     * @field STATUS_REQUIRES_USER_CONFIRMATION
     */
    SETCONSTANT(STATUS_REQUIRES_USER_CONFIRMATION, 9)



    /** 
     * @field ERRORCODE_NO_ERROR
     */
    SETCONSTANT(ERRORCODE_NO_ERROR, 0)

    /** The requesting app is unavailable.
     * @field ERRORCODE_APP_UNAVAILABLE
     */
    SETCONSTANT(ERRORCODE_APP_UNAVAILABLE, -1)

    /** The requested asset pack isn't available.
     * @field ERRORCODE_PACK_UNAVAILABLE
     */
    SETCONSTANT(ERRORCODE_PACK_UNAVAILABLE, -2)

    /** The request is invalid.
     * @field ERRORCODE_INVALID_REQUEST
     */
    SETCONSTANT(ERRORCODE_INVALID_REQUEST, -3)

    /** The requested download isn't found.
     * @field ERRORCODE_DOWNLOAD_NOT_FOUND
     */
    SETCONSTANT(ERRORCODE_DOWNLOAD_NOT_FOUND, -4)

    /** The Asset Delivery API isn't available.
     * @field ERRORCODE_API_NOT_AVAILABLE
     */
    SETCONSTANT(ERRORCODE_API_NOT_AVAILABLE, -5)

    /** Network error.
     * @field ERRORCODE_NETWORK_ERROR
     */
    SETCONSTANT(ERRORCODE_NETWORK_ERROR, -6)

    /** Download not permitted under the current device circumstances
     * @field ERRORCODE_ACCESS_DENIED
     */
    SETCONSTANT(ERRORCODE_ACCESS_DENIED, -7)

    /** Asset pack download failed due to insufficient storage.
     * @field ERRORCODE_INSUFFICIENT_STORAGE
     */
    SETCONSTANT(ERRORCODE_INSUFFICIENT_STORAGE, -10)

    /** The Play Store app is either not installed or not the official version.
     * @field ERRORCODE_PLAY_STORE_NOT_FOUND
     */
    SETCONSTANT(ERRORCODE_PLAY_STORE_NOT_FOUND, -11)

    /** Returned if AssetPackManager.showCellularDataConfirmation(Activity) is called but no asset packs are waiting for Wi-Fi.
     * @field ERRORCODE_NETWORK_UNRESTRICTED
     */
    SETCONSTANT(ERRORCODE_NETWORK_UNRESTRICTED, -12)

    /** The app isn't owned by any user on this device.
     * @field ERRORCODE_APP_NOT_OWNED
     */
    SETCONSTANT(ERRORCODE_APP_NOT_OWNED, -13)

    /** Returned if AssetPackManager.showConfirmationDialog(Activity) is called but no asset packs require user confirmation.
     * @field ERRORCODE_CONFIRMATION_NOT_REQUIRED
     */
    SETCONSTANT(ERRORCODE_CONFIRMATION_NOT_REQUIRED, -14)

    /** The installed app version is not recognized by Play.
     * @field ERRORCODE_UNRECOGNIZED_INSTALLATION
     */
    SETCONSTANT(ERRORCODE_UNRECOGNIZED_INSTALLATION, -15)

    /** Unknown error downloading an asset pack.
     * @field ERRORCODE_INTERNAL_ERROR
     */
    SETCONSTANT(ERRORCODE_INTERNAL_ERROR, -100)


    #undef SETCONSTANT


    lua_pop(L, 1);
    assert(top == lua_gettop(L));
}


static void JavaInit() {
    dmLogInfo("JavaInit");
    dmAndroid::ThreadAttacher threadAttacher;
    JNIEnv* env = threadAttacher.GetEnv();
    jclass cls = dmAndroid::LoadClass(env, "com.defold.pad.PADJNI");

    jmethodID jni_constructor = env->GetMethodID(cls, "<init>", "(Landroid/app/Activity;)V");
    g_PAD.m_JNI = env->NewGlobalRef(env->NewObject(cls, jni_constructor, threadAttacher.GetActivity()->clazz));

    g_PAD.m_Cancel = env->GetMethodID(cls, "Cancel", "(Ljava/lang/String;)V");
    g_PAD.m_Fetch = env->GetMethodID(cls, "Fetch", "(Ljava/lang/String;)V");
    g_PAD.m_GetPackState = env->GetMethodID(cls, "GetPackState", "(Ljava/lang/String;)V");
    g_PAD.m_HasPackState = env->GetMethodID(cls, "HasPackState", "(Ljava/lang/String;)Z");
    g_PAD.m_RemovePack = env->GetMethodID(cls, "RemovePack", "(Ljava/lang/String;)V");
    g_PAD.m_ShowConfirmationDialog = env->GetMethodID(cls, "ShowConfirmationDialog", "(Ljava/lang/String;)V");
    g_PAD.m_GetNextEvent = env->GetMethodID(cls, "GetNextEvent", "()Ljava/lang/String;");
    g_PAD.m_GetPackBytesDownloaded = env->GetMethodID(cls, "GetPackBytesDownloaded", "(Ljava/lang/String;)J");
    g_PAD.m_GetPackErrorCode = env->GetMethodID(cls, "GetPackErrorCode", "(Ljava/lang/String;)I");
    g_PAD.m_GetPackStatus = env->GetMethodID(cls, "GetPackStatus", "(Ljava/lang/String;)I");
    g_PAD.m_GetPackTotalBytesToDownload = env->GetMethodID(cls, "GetPackTotalBytesToDownload", "(Ljava/lang/String;)J");
    g_PAD.m_GetPackTransferProgressPercentage = env->GetMethodID(cls, "GetPackTransferProgressPercentage", "(Ljava/lang/String;)I");
    g_PAD.m_GetPackLocation = env->GetMethodID(cls, "GetPackLocation", "(Ljava/lang/String;)Ljava/lang/String;");
}


#endif


dmExtension::Result AppInitializePAD(dmExtension::AppParams* params)
{
    dmLogInfo("AppInitializePAD");
    return dmExtension::RESULT_OK;
}

dmExtension::Result InitializePAD(dmExtension::Params* params)
{
#if !defined(DM_PLATFORM_ANDROID)
    dmLogInfo("Extension %s is not supported", LIB_NAME);
#else
    dmLogInfo("Registered %s Extension", MODULE_NAME);
    LuaInit(params->m_L);
    JavaInit();
#endif
    return dmExtension::RESULT_OK;
}

dmExtension::Result AppFinalizePAD(dmExtension::AppParams* params)
{
    dmLogInfo("AppFinalizePAD");
    return dmExtension::RESULT_OK;
}

dmExtension::Result FinalizePAD(dmExtension::Params* params)
{
    dmLogInfo("FinalizePAD");
#if defined(DM_PLATFORM_ANDROID)
    if (g_PADListener)
    {
        dmScript::DestroyCallback(g_PADListener);
        g_PADListener = 0;
    }
#endif
    return dmExtension::RESULT_OK;
}

dmExtension::Result OnUpdatePAD(dmExtension::Params* params)
{
#if defined(DM_PLATFORM_ANDROID)
    while (const char* event_json = CallCharMethodVoid(g_PAD.m_JNI, g_PAD.m_GetNextEvent))
    {
        dmLogInfo("OnUpdatePAD event %s", event_json);
        HandleEvent(event_json);
    }
#endif
    return dmExtension::RESULT_OK;
}

void OnEventPAD(dmExtension::Params* params, const dmExtension::Event* event)
{
    switch(event->m_Event)
    {
        case dmExtension::EVENT_ID_ACTIVATEAPP:
            dmLogInfo("OnEventPAD - EVENT_ID_ACTIVATEAPP");
            break;
        case dmExtension::EVENT_ID_DEACTIVATEAPP:
            dmLogInfo("OnEventPAD - EVENT_ID_DEACTIVATEAPP");
            break;
        case dmExtension::EVENT_ID_ICONIFYAPP:
            dmLogInfo("OnEventPAD - EVENT_ID_ICONIFYAPP");
            break;
        case dmExtension::EVENT_ID_DEICONIFYAPP:
            dmLogInfo("OnEventPAD - EVENT_ID_DEICONIFYAPP");
            break;
        default:
            dmLogWarning("OnEventPAD - Unknown event id");
            break;
    }
}

DM_DECLARE_EXTENSION(PAD, LIB_NAME, AppInitializePAD, AppFinalizePAD, InitializePAD, OnUpdatePAD, OnEventPAD, FinalizePAD)
