/*

	ParticleRainApp.h
		- Shuvashis Das 02/13/2020

*/

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"

#include "cinder/Capture.h"
#include "OpticalFlow.h"
#include "ParticleRain.h"

using namespace ci;
using namespace ci::app;
using namespace std;

int										appScreenWidth = 1920;
int										appScreenHeight = 1080;


class ParticleRainApp : public App {
public:
	void								setup() override;
	void								update() override;
	void								draw() override;
	gl::TextureRef						flipCamTexture();

	// Optical Flow variables
	float								mLambda;
	float								mBlurAmount;
	float								mDisplacement;
	CaptureRef							mCapture;
	gl::TextureRef						mDefaultCamTexture;
	gl::FboRef							mCamTexFlippedFbo;
	OpticalFlow							mOpticalFlow;
	int									mCamResX = 640;
	int									mCamResY = 480;

	// Particle system variables 
	int									mParticleRangeX = mCamResX * 2;
	int									mParticleRangeY = mCamResY * 2;
	


private:
	enum { WORK_GROUP_SIZE = 128, };
	gl::GlslProgRef						mRenderProg;
	gl::GlslProgRef						mUpdateProg;

	// Buffers holding raw particle data on GPU.
	gl::SsboRef							mParticleBuffer;
	gl::VboRef							mIdsVbo;
	gl::VaoRef							mAttributes;

	ParticleRain						mParticleRain;
};

void ParticleRainApp::setup() {
	gl::enableAlphaBlending();

	// Optical Flow setup
	try {
		mCapture = Capture::create(mCamResX, mCamResY);
		mCapture->start();
		CI_LOG_I("Default camera started");
	}
	catch (ci::Exception & exc) {
		CI_LOG_EXCEPTION("Failed to init capture ", exc);
	}

	gl::Fbo::Format format;
	format.setColorTextureFormat(gl::Texture::Format().internalFormat(GL_RGBA));
	mCamTexFlippedFbo = gl::Fbo::create(mCamResX, mCamResY, format);

	mOpticalFlow.setup(mCamResX, mCamResY);
	mOpticalFlow.setLambda(0.1);
	mOpticalFlow.setBlur(0.5);
	mOpticalFlow.setDisplacement(0.05f);

	ivec3 count = gl::getMaxComputeWorkGroupCount();
	CI_ASSERT(count.x >= (mNumParticles / WORK_GROUP_SIZE));


	mParticleRain.setup(mParticleRangeX, mParticleRangeY);

	// Listen to mouse events so we can send data as uniforms.
	getWindow()->getSignalMouseDown().connect([this](MouseEvent event)
	{
		mParticleRain.mMouseDown = true;
		mParticleRain.mMouseForce = 500.0f;
		mParticleRain.mMousePos = vec3(event.getX(), event.getY(), 0.0f);
	});
	getWindow()->getSignalMouseDrag().connect([this](MouseEvent event)
	{
		mParticleRain.mMousePos = vec3(event.getX(), event.getY(), 0.0f);
	});
	getWindow()->getSignalMouseUp().connect([this](MouseEvent event)
	{
		mParticleRain.mMouseForce = 0.0f;
		mParticleRain.mMouseDown = false;
	});
}

gl::TextureRef ParticleRainApp::flipCamTexture() {
	gl::clear(Color::black());
	gl::ScopedFramebuffer fbScpLastTex(mCamTexFlippedFbo);
	gl::ScopedViewport scpVpLastTex(ivec2(0), mCamTexFlippedFbo->getSize());
	gl::ScopedMatrices matFlipTex;
	gl::setMatricesWindow(mCamTexFlippedFbo->getSize());
	gl::scale(-1, 1);
	gl::translate(-mCamTexFlippedFbo->getWidth(), 0);
	gl::draw(mDefaultCamTexture, mCamTexFlippedFbo->getBounds());
	return mCamTexFlippedFbo->getColorTexture();
}

void ParticleRainApp::update() {
	//optical flow update
	if (mCapture && mCapture->checkNewFrame()) {
		if (!mDefaultCamTexture) {
			// Capture images come back as top-down, and it's more efficient to keep them that way
			mDefaultCamTexture = gl::Texture::create(*mCapture->getSurface(), gl::Texture::Format());
		}
		else {
			mDefaultCamTexture->update(*mCapture->getSurface());
			mOpticalFlow.update(flipCamTexture());
		}
	}

	mParticleRain.update( mOpticalFlow.getOpticalFlowTexture() );
}

void ParticleRainApp::draw() {
	gl::clear(Color(0, 0, 0));

	// draw camera feed
	{
		// use this section to see the optical flow texture instead of the camera feed behind the particles
		//gl::ScopedMatrices matOpticalFlow;
		//gl::scale(mParticleRangeX / mCamResX, mParticleRangeY / mCamResY);
		//mOpticalFlow.drawFlowGrid();

		gl::ScopedMatrices matCamFeed;
		Rectf bounds(0, 0, mParticleRangeX, mParticleRangeY);
		gl::draw(flipCamTexture(), bounds);

	}

	// darw particle system on top of camera feed
	{
		mParticleRain.draw();
	}
	

	// draw optical flow at top right corner of app window
	{
		gl::ScopedMatrices matOpticalFlow;
		gl::translate(appScreenWidth - mOpticalFlow.getWidth(), 0);
		mOpticalFlow.drawFlowGrid();
	}

	// log FPS on screen
	{
		gl::drawString(toString(static_cast<int>(getAverageFps())) + " fps", vec2(appScreenWidth - 100, appScreenHeight - 100));
	}
}

CINDER_APP(ParticleRainApp, RendererGl, [](App::Settings* settings) {
	settings->setWindowSize(appScreenWidth, appScreenHeight);
	settings->setMultiTouchEnabled(false);
})
