#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

// Settings
#include "SDASettings.h"
// Session
#include "SDASession.h"
// Log
#include "SDALog.h"
// Spout
#include "CiSpoutOut.h"
// UI
#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS 1
#include "SDAUI.h"
#define IM_ARRAYSIZE(_ARR)			((int)(sizeof(_ARR)/sizeof(*_ARR)))

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace SophiaDigitalArt;

class MassActivApp : public App {

public:
	MassActivApp();
	void mouseMove(MouseEvent event) override;
	void mouseDown(MouseEvent event) override;
	void mouseDrag(MouseEvent event) override;
	void mouseUp(MouseEvent event) override;
	void keyDown(KeyEvent event) override;
	void keyUp(KeyEvent event) override;
	void fileDrop(FileDropEvent event) override;
	void update() override;
	void draw() override;
	void cleanup() override;
	void setUIVisibility(bool visible);
private:
	// Settings
	SDASettingsRef					mSDASettings;
	// Session
	SDASessionRef					mSDASession;
	// Log
	SDALogRef						mSDALog;
	// UI
	SDAUIRef						mSDAUI;
	// handle resizing for imgui
	void							resizeWindow();
	// imgui
	float							color[4];
	float							backcolor[4];
	int								playheadPositions[12];
	int								speeds[12];

	float							f = 0.0f;
	char							buf[64];
	unsigned int					i, j;

	bool							mouseGlobal;

	string							mError;
	// fbo
	bool							mIsShutDown;
	Anim<float>						mRenderWindowTimer;
	void							positionRenderWindow();
	bool							mFadeInDelay;
	SpoutOut 						mSpoutOut;
	int								mode;
	//! fbos
	void							renderToFbo();
	gl::FboRef						mFbo;
	//! shaders
	gl::GlslProgRef					mGlsl;
	bool							mUseShader;
};


MassActivApp::MassActivApp()
	: mSpoutOut("MassActiv", app::getWindowSize())
{
	// Settings
	mSDASettings = SDASettings::create("MassActiv");
	// Session
	mSDASession = SDASession::create(mSDASettings);
	//mSDASettings->mCursorVisible = true;
	setUIVisibility(mSDASettings->mCursorVisible);
	mSDASession->getWindowsResolution();

	mouseGlobal = false;
	mFadeInDelay = true;
	// UI
	mSDAUI = SDAUI::create(mSDASettings, mSDASession);
	// windows
	mIsShutDown = false;
	mRenderWindowTimer = 0.0f;
	mode = 0;
	//timeline().apply(&mRenderWindowTimer, 1.0f, 2.0f).finishFn([&] { positionRenderWindow(); });
		// fbo
	gl::Fbo::Format format;
	//format.setSamples( 4 ); // uncomment this to enable 4x antialiasing
	mFbo = gl::Fbo::create(mSDASettings->mRenderWidth, mSDASettings->mRenderHeight, format.depthTexture());
	// shader
	mUseShader = true;
	mGlsl = gl::GlslProg::create(gl::GlslProg::Format().vertex(loadAsset("passthrough.vs")).fragment(loadAsset("post.glsl")));

	gl::enableDepthRead();
	gl::enableDepthWrite();
}
void MassActivApp::resizeWindow()
{
	mSDAUI->resize();
}
void MassActivApp::positionRenderWindow() {
	mSDASettings->mRenderPosXY = ivec2(mSDASettings->mRenderX, mSDASettings->mRenderY);//20141214 was 0
	setWindowPos(mSDASettings->mRenderX, mSDASettings->mRenderY);
	setWindowSize(mSDASettings->mRenderWidth, mSDASettings->mRenderHeight);
}
void MassActivApp::setUIVisibility(bool visible)
{
	if (visible)
	{
		showCursor();
	}
	else
	{
		hideCursor();
	}
}
void MassActivApp::fileDrop(FileDropEvent event)
{
	mSDASession->fileDrop(event);
}

void MassActivApp::cleanup()
{
	if (!mIsShutDown)
	{
		mIsShutDown = true;
		CI_LOG_V("shutdown");
		// save settings
		mSDASettings->save();
		mSDASession->save();
		quit();
	}
}
void MassActivApp::mouseMove(MouseEvent event)
{
	if (!mSDASession->handleMouseMove(event)) {
		// let your application perform its mouseMove handling here
	}
}
void MassActivApp::mouseDown(MouseEvent event)
{
	if (!mSDASession->handleMouseDown(event)) {
		// let your application perform its mouseDown handling here
		if (event.isRightDown()) {
		}
	}
}
void MassActivApp::mouseDrag(MouseEvent event)
{
	if (!mSDASession->handleMouseDrag(event)) {
		// let your application perform its mouseDrag handling here
	}
}
void MassActivApp::mouseUp(MouseEvent event)
{
	if (!mSDASession->handleMouseUp(event)) {
		// let your application perform its mouseUp handling here
	}
}

void MassActivApp::keyDown(KeyEvent event)
{
	if (!mSDASession->handleKeyDown(event)) {
		switch (event.getCode()) {
		case KeyEvent::KEY_m:
			mUseShader = !mUseShader;
			break;
		case KeyEvent::KEY_h:
			// mouse cursor and ui visibility
			mSDASettings->mCursorVisible = !mSDASettings->mCursorVisible;
			setUIVisibility(mSDASettings->mCursorVisible);
			break;
		}
	}
}
void MassActivApp::keyUp(KeyEvent event)
{
	if (!mSDASession->handleKeyUp(event)) {
	}
}
// Render into the FBO
void MassActivApp::renderToFbo()
{

	// this will restore the old framebuffer binding when we leave this function
	// on non-OpenGL ES platforms, you can just call mFbo->unbindFramebuffer() at the end of the function
	// but this will restore the "screen" FBO on OpenGL ES, and does the right thing on both platforms
	gl::ScopedFramebuffer fbScp(mFbo);
	// clear out the FBO with black
	gl::clear(Color::black());

	// setup the viewport to match the dimensions of the FBO
	gl::ScopedViewport scpVp(ivec2(0), mFbo->getSize());

	// render

	// texture binding must be before ScopedGlslProg
	mSDASession->getMixetteTexture()->bind(0);

	/*mSDASession->getFboTexture(5)->bind(0);
	mSDASession->getFboTexture(6)->bind(1);
	mSDASession->getFboTexture(7)->bind(2); */

	gl::ScopedGlslProg prog(mGlsl);

	mGlsl->uniform("iGlobalTime", (float)getElapsedSeconds());
	mGlsl->uniform("iResolution", vec3(mSDASettings->mRenderWidth, mSDASettings->mRenderHeight, 1.0));
	mGlsl->uniform("iChannel0", 0); // texture 0
	mGlsl->uniform("iChannel1", 1);
	//mGlsl->uniform("iChannel2", 2);
	/*mGlsl->uniform("iExposure", 1.0f);
mGlsl->uniform("iSobel", 1.0f);
	mGlsl->uniform("iChromatic", 1.0f);		*/
	mGlsl->uniform("iExposure", mSDASession->getFloatUniformValueByName("iExposure"));
	mGlsl->uniform("iSobel", mSDASession->getFloatUniformValueByName("iSobel"));
	mGlsl->uniform("iChromatic", mSDASession->getFloatUniformValueByName("iChromatic"));

	gl::drawSolidRect(getWindowBounds());

}
void MassActivApp::update()
{
	mSDASession->setFloatUniformValueByIndex(mSDASettings->IFPS, getAverageFps());
	mSDASession->update();
	// render into our FBO
	if (mUseShader) {
		renderToFbo();
	}
}
void MassActivApp::draw()
{
	gl::clear(Color::black());
	if (mFadeInDelay) {
		mSDASettings->iAlpha = 0.0f;
		if (getElapsedFrames() > mSDASession->getFadeInDelay()) {
			mFadeInDelay = false;
			timeline().apply(&mSDASettings->iAlpha, 0.0f, 1.0f, 1.5f, EaseInCubic());
		}
	}
	mode = mSDASession->getMode();
	CI_LOG_V("mode " + toString(mode));
	gl::setMatricesWindow(toPixels(getWindowSize()), true);
	//gl::setMatricesWindow(mSDASettings->mRenderWidth, mSDASettings->mRenderHeight, true);
	if (mUseShader) {
		gl::draw(mFbo->getColorTexture(), getWindowBounds());
	}
	else {

		switch (mode)
		{
		case 1:
			gl::draw(mSDASession->getMixTexture(), getWindowBounds());
			break;
		case 2:
			gl::draw(mSDASession->getRenderTexture(), getWindowBounds());
			break;
		case 3:
			gl::draw(mSDASession->getHydraTexture(), getWindowBounds());
			break;
		case 4:
			gl::draw(mSDASession->getFboTexture(0), getWindowBounds());
			break;
		case 5:
			gl::draw(mSDASession->getFboTexture(1), getWindowBounds());
			break;
		case 6:
			gl::draw(mSDASession->getFboTexture(2), getWindowBounds());
			break;
		case 7:
			gl::draw(mSDASession->getFboTexture(3), getWindowBounds());
			break;
		case 8:
			gl::draw(mSDASession->getMixetteTexture(), getWindowBounds());
			break;
		default:
			gl::draw(mSDASession->getMixetteTexture(), getWindowBounds());

			break;
		}
	}

	// Spout Send
	mSpoutOut.sendViewport();
	if (mSDASettings->mCursorVisible) {
		mSDAUI->Run("UI", (int)getAverageFps());
		if (mSDAUI->isReady()) {
		}
	}
	getWindow()->setTitle(mSDASettings->sFps + " MassActiv");
}

void prepareSettings(App::Settings *settings)
{
	settings->setWindowSize(1280, 720);
#ifdef _DEBUG
	settings->setConsoleWindowEnabled();
#else
#endif  // _DEBUG
}

CINDER_APP(MassActivApp, RendererGl, prepareSettings)
