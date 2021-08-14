
#include <EGL/egl.h>
#include <coreutils/log.h>

#ifdef ANDROID
int32_t ANativeWindow_setBuffersGeometry(ANativeWindow* window, int32_t width, int32_t height, int32_t format);
#endif

bool initEGL(EGLConfig& eglConfig, EGLContext& eglContext, EGLDisplay& eglDisplay, EGLSurface &eglSurface, EGLNativeWindowType nativeWin) {

	//EGLint format;
	EGLint numConfigs;
	EGLConfig config;
	EGLConfig configList[32];
	//EGLContext context;

	eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);

	EGLint m0;
	EGLint m1;

	eglInitialize(eglDisplay, &m0, &m1);

	LOGI("EGL v%d.%d", m0, m1);	

	/* Here, the application chooses the configuration it desires. In this
	 * sample, we have a very simplified selection process, where we pick
	 * the first EGLConfig that matches our criteria */
	eglGetConfigs(eglDisplay, configList, 32, &numConfigs);

	LOGI("Found %d matching configs", numConfigs);

	for(int i=0; i<numConfigs; i++) {
		EGLint conf, id, stype, redSize, caveat, sbuffers;
		eglGetConfigAttrib(eglDisplay, configList[i], EGL_CONFORMANT, &conf);
		eglGetConfigAttrib(eglDisplay, configList[i], EGL_CONFIG_ID, &id);
		eglGetConfigAttrib(eglDisplay, configList[i], EGL_SURFACE_TYPE, &stype);
		eglGetConfigAttrib(eglDisplay, configList[i], EGL_RED_SIZE, &redSize);
		eglGetConfigAttrib(eglDisplay, configList[i], EGL_CONFIG_CAVEAT, &caveat);
		eglGetConfigAttrib(eglDisplay, configList[i], EGL_SAMPLE_BUFFERS, &sbuffers);
						
		LOGI("Config %d (%d) conformant %x RED %d caveat %x stype %x", i, id, conf, redSize, caveat, stype);

		if((conf & EGL_OPENGL_ES2_BIT) && (stype & EGL_WINDOW_BIT)) {
			config = configList[i];
			LOGD("Found config");
			if(sbuffers > 0) {
				break;
			}
		}
	}

	const EGLint attribs[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2, 
		EGL_NONE, EGL_NONE
	};
/*
   EGLint attribList[] =
   {
       EGL_RED_SIZE,       8,
       EGL_GREEN_SIZE,     8,
       EGL_BLUE_SIZE,      8,
       EGL_ALPHA_SIZE,     8,
       EGL_DEPTH_SIZE,     8,
       EGL_STENCIL_SIZE,   EGL_DONT_CARE,
       EGL_SAMPLE_BUFFERS, 0,
       EGL_NONE
   };
 

   if (!eglGetConfigs(eglDisplay, NULL, 0, &numConfigs)) {
   		LOGD("Fail get");
		return false;
   }

   if(!eglChooseConfig(eglDisplay, attribList, &config, 1, &numConfigs)) {
   		LOGD("Fail choose");
		return false;
   }
*/
	eglContext = eglCreateContext(eglDisplay, config, nullptr, attribs);
	if(eglContext == EGL_NO_CONTEXT) {
		LOGI("NO CONTEXT!");
		return false;
	}


#ifdef ANDROID
	/* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
	 * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
	 * As soon as we picked a EGLConfig, we can safely reconfigure the
	 * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
	EGLint visid;
	eglGetConfigAttrib(eglDisplay, config, EGL_NATIVE_VISUAL_ID, &visid);

	LOGI("Native id %d", visid);
	ANativeWindow_setBuffersGeometry(nativeWin, 0, 0, visid);
#endif

   eglSurface = eglCreateWindowSurface(eglDisplay, config, nativeWin, nullptr);
   if(eglSurface == EGL_NO_SURFACE) {
		LOGI("NO SURFACE!");
      return false;
   }

	if(eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext) == EGL_FALSE) {
		LOGI("NO MAKE CURRENT!");
		return false;
	}

	eglConfig = config;
	LOGD("EGL INIT DONE");
	return true;
}
