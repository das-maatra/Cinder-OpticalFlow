/*

	ParticleGridApp.h
		- Shuvashis Das 02/13/2020

*/

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Rand.h"
#include "cinder/Utilities.h"

#include "cinder/Capture.h"
#include "OpticalFlow.h"

using namespace ci;
using namespace ci::app;
using namespace std;

int										appScreenWidth = 1920;
int										appScreenHeight = 1080;

#pragma pack( push, 1 )
struct Particle
{
	vec3								pos;
	float   							pad1;
	vec3								ppos;
	float   							pad2;
	vec3								initPos;
	float   							pad3;
	vec4    							color;
	float								damping;
	vec3    							pad4;
};
#pragma pack( pop )


class ParticleGridApp : public App {
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
	float								mParticleSize = 5.0;
	int									mParticlesAlongX = 32;
	int									mParticlesAlongY = 24;
	int									mParticleGapX = mParticleRangeX / mParticlesAlongX;
	int									mParticleGapY = mParticleRangeY / mParticlesAlongY;
	int									mParticlePosOffsetX = (mParticleRangeX / mParticlesAlongX) / 2;
	int									mParticlePosOffsetY = (mParticleRangeY / mParticlesAlongY) / 2;
	const int							mNumParticles = static_cast<int>(mParticlesAlongX * mParticlesAlongY);
	float								mForceMult = 10.0;



private:
	enum { WORK_GROUP_SIZE = 128, };
	gl::GlslProgRef						mRenderProg;
	gl::GlslProgRef						mUpdateProg;

	// Buffers holding raw particle data on GPU.
	gl::SsboRef							mParticleBuffer;
	gl::VboRef							mIdsVbo;
	gl::VaoRef							mAttributes;
};

void ParticleGridApp::setup() {
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

	// Particle system setup.
	vector<Particle> particles;
	particles.assign(mNumParticles, Particle());


	for (int i = 0; i < mParticlesAlongX; i++) {
		for (int j = 0; j < mParticlesAlongY; j++) {
			float x = (mParticleGapX * i) + mParticlePosOffsetX;
			float y = (mParticleGapY * j) + mParticlePosOffsetY;
			float z = 0.0;
			// CI_LOG_I(mParticlesAlongX * j + i);
			auto& p = particles.at(mParticlesAlongX * j + i);
			p.pos = vec3(x, y, z);
			p.initPos = p.pos;
			p.ppos = p.initPos;
			p.damping = 0.85f;
			p.color = vec4(1.0);
		}
	}

	ivec3 count = gl::getMaxComputeWorkGroupCount();
	CI_ASSERT(count.x >= (mNumParticles / WORK_GROUP_SIZE));

	// Create particle buffers on GPU and copy data into the first buffer.
	// Mark as static since we only write from the CPU once.
	mParticleBuffer = gl::Ssbo::create(particles.size() * sizeof(Particle), particles.data(), GL_STATIC_DRAW);
	gl::ScopedBuffer scopedParticleSsbo(mParticleBuffer);
	mParticleBuffer->bindBase(0);

	// Create a default color shader.
	try {
		mRenderProg = gl::GlslProg::create(gl::GlslProg::Format().vertex(loadAsset("glsl/particles/particleRender.vert"))
			.fragment(loadAsset("glsl/particles/particleRender.frag"))
			.geometry(loadAsset("glsl/particles/particleRender.geom"))
			.attribLocation("particleId", 0));
	}
	catch (gl::GlslProgCompileExc e) {
		ci::app::console() << e.what() << std::endl;
		quit();
	}

	std::vector<GLuint> ids(mNumParticles);
	GLuint currId = 0;
	std::generate(ids.begin(), ids.end(), [&currId]() -> GLuint { return currId++; });

	mIdsVbo = gl::Vbo::create<GLuint>(GL_ARRAY_BUFFER, ids, GL_STATIC_DRAW);
	mAttributes = gl::Vao::create();
	gl::ScopedVao vao(mAttributes);
	gl::ScopedBuffer scopedIds(mIdsVbo);
	gl::enableVertexAttribArray(0);
	gl::vertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(GLuint), 0);

	try {
		// Load our update program.
		mUpdateProg = gl::GlslProg::
			create(gl::GlslProg::Format().compute(loadAsset("glsl/particles/particleUpdate.comp")));
	}
	catch (gl::GlslProgCompileExc e) {
		ci::app::console() << e.what() << std::endl;
		quit();
	}
}

gl::TextureRef ParticleGridApp::flipCamTexture() {
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

void ParticleGridApp::update() {
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

	// Update particles on the GPU
	gl::ScopedGlslProg prog(mUpdateProg);

	gl::ScopedTextureBind texScope(mOpticalFlow.getOpticalFlowBlurTexture(), 0);
	mUpdateProg->uniform("uCanvasSize", vec2(mParticleRangeX, mParticleRangeY));
	mUpdateProg->uniform("uForceMult", mForceMult);

	gl::ScopedBuffer scopedParticleSsbo(mParticleBuffer);

	gl::dispatchCompute(mNumParticles / WORK_GROUP_SIZE, 1, 1);
	gl::memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void ParticleGridApp::draw() {
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
		gl::ScopedGlslProg render(mRenderProg);
		gl::ScopedBuffer scopedParticleSsbo(mParticleBuffer);
		gl::ScopedVao vao(mAttributes);
		glEnable(GL_PROGRAM_POINT_SIZE);
		mRenderProg->uniform("uParticleSize", mParticleSize);
		gl::context()->setDefaultShaderVars();
		gl::drawArrays(GL_POINTS, 0, mNumParticles);
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

CINDER_APP(ParticleGridApp, RendererGl, [](App::Settings * settings) {
	settings->setWindowSize(appScreenWidth, appScreenHeight);
	settings->setMultiTouchEnabled(false);
})
