#include "ParticleRain.h"
//ParticleRain::ParticleRain() {
//}

void ParticleRain::setup(int resX, int resY ){

	mResX = resX;
	mResY = resY;
	// Create initial particle layout.
	vector<Particle> particles;
	particles.assign(NUM_PARTICLES, Particle());
	const float azimuth = 256.0f * static_cast<float>(M_PI) / particles.size();
	const float inclination = static_cast<float>(M_PI) / particles.size();
	const float radius = 180.0f;
	vec3 center = vec3(getWindowCenter() + vec2(0.0f, 40.0f), 0.0f);

	for (unsigned int i = 0; i < particles.size(); ++i) {

		float x, y, z;
		// assign starting values to particles.
		x = Rand::randFloat((-mResX/2), (mResX/2)) - 320;
		y = Rand::randFloat((-mResY / 2) - 50, (mResY / 2) + 50);
		z = Rand::randFloat(0.5, 1.0);


		auto& p = particles.at(i);
		p.pos = center + vec3(x, y, z);
		p.home = p.pos;
		p.ppos = p.home + Rand::randVec3() * 10.0f; // random initial velocity
		p.damping = Rand::randFloat(0.25f, 1.5f);
		p.scale = Rand::randFloat(0.1f, 20.0f);
		Color c(CM_HSV, lmap<float>(static_cast<float>(i), 0.0f, static_cast<float>(particles.size()), 0.10f, 0.66f), 1.0f, 1.0f);
		p.color = vec4(c.r, c.g, c.b, 1.0f);
	}

	ivec3 count = gl::getMaxComputeWorkGroupCount();
	CI_ASSERT(count.x >= (NUM_PARTICLES / WORK_GROUP_SIZE));

	// Create particle buffers on GPU and copy data into the first buffer.
	// Mark as static since we only write from the CPU once.
	mParticleBuffer = gl::Ssbo::create(particles.size() * sizeof(Particle), particles.data(), GL_STATIC_DRAW);
	gl::ScopedBuffer scopedParticleSsbo(mParticleBuffer);
	mParticleBuffer->bindBase(0);

	// Create a default color shader.
	mRenderProg = gl::GlslProg::create(gl::GlslProg::Format().vertex(loadAsset("glsl/particles/particleRender.vert"))
		.fragment(loadAsset("glsl/particles/particleRender.frag"))
		.attribLocation("particleId", 0));

	std::vector<GLuint> ids(NUM_PARTICLES);
	GLuint currId = 0;
	std::generate(ids.begin(), ids.end(), [&currId]() -> GLuint { return currId++; });

	mIdsVbo = gl::Vbo::create<GLuint>(GL_ARRAY_BUFFER, ids, GL_STATIC_DRAW);
	mAttributes = gl::Vao::create();
	gl::ScopedVao vao(mAttributes);
	gl::ScopedBuffer scopedIds(mIdsVbo);
	gl::enableVertexAttribArray(0);
	gl::vertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(GLuint), 0);

	mUpdateProg = gl::GlslProg::create(gl::GlslProg::Format().compute(loadAsset("glsl/particles/particleUpdate.comp")));

	// loading particle texture
	auto img = loadImage(loadAsset("glsl/textures/particle_circle.png"));
	mParticleTexture = gl::Texture2d::create(img);
	mParticleTexture->bind(0);
}

void ParticleRain::update(ci::gl::Texture2dRef opticalFlowTex )
{
	// Update particles on the GPU
	gl::ScopedGlslProg prog(mUpdateProg);

	gl::ScopedTextureBind texScope(opticalFlowTex, 0);
	mUpdateProg->uniform("uCanvasSize", vec2(mResX, mResY));
	mUpdateProg->uniform("uForceMult", mForceMult);
	mUpdateProg->uniform("uMouseForce", mMouseForce);
	mUpdateProg->uniform("uMousePos", mMousePos);
	mUpdateProg->uniform("uWindowHeight", mWindowHeight);
	mUpdateProg->uniform("uGravity", mGravity);
	gl::ScopedBuffer scopedParticleSsbo(mParticleBuffer);

	gl::dispatchCompute(NUM_PARTICLES / WORK_GROUP_SIZE, 1, 1);
	gl::memoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	// Update mouse force.
	if (mMouseDown) {
		mMouseForce = 150.0f;
	}
}

void ParticleRain::draw()
{
	//gl::clear(Color(0, 0, 0));
	gl::setMatricesWindowPersp(getWindowSize());

	gl::ScopedGlslProg			render(mRenderProg);
	gl::ScopedTextureBind		texScope(mParticleTexture);
	gl::ScopedBuffer			scopedParticleSsbo(mParticleBuffer);
	gl::ScopedState				stateScope(GL_PROGRAM_POINT_SIZE, true);
	gl::ScopedVao				vao(mAttributes);

	gl::ScopedMatrices();
	gl::setDefaultShaderVars();
	gl::drawArrays(GL_POINTS, 0, NUM_PARTICLES);
	mRenderProg->uniform("uParticleTexture", 0);
	gl::setMatricesWindow(app::getWindowSize());
}
