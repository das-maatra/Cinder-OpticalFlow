#include "Shaders.h"

void Shaders::setup() {

	try {
		
		mFlowShader = ci::gl::GlslProg::create(ci::gl::GlslProg::Format()
			.vertex(CI_GLSL(150,
				uniform mat4 ciModelViewProjection;

				in vec4 ciPosition;
				in vec2 ciTexCoord0;

				out vec2 vTexCoord0;

				void main()
				{
					vTexCoord0 = ciTexCoord0;
					gl_Position = ciModelViewProjection * ciPosition;
				}
			))
			.fragment(CI_GLSL(150,
				uniform sampler2D tex0;
				uniform sampler2D tex1;


				uniform vec2 uScale;
				uniform vec2 uOffset;
				uniform float uLambda;
				in vec2 vTexCoord0;

				out vec4 oColor;

				vec4 getColorCoded(float x, float y, vec2 uScale) {
					vec2 xout = vec2(max(x, 0.), abs(min(x, 0.))) * uScale.x;
					vec2 yout = vec2(max(y, 0.), abs(min(y, 0.))) * uScale.y;
					float dirY = 1;
					if (yout.x > yout.y)  dirY = 0.90;
					return vec4(xout.xy, max(yout.x, yout.y), dirY);
				}


				vec4 getGrayScale(vec4 col) {
					float gray = dot(vec3(col.x, col.y, col.z), vec3(0.3, 0.59, 0.11));
					return vec4(gray, gray, gray, 1);
				}

				vec4 texture2DRectGray(sampler2D tex, vec2 coord) {
					return getGrayScale(texture(tex, coord));
				}

				void main()
				{
					vec4 a = texture2DRectGray(tex0, vTexCoord0);

					vec4 b = texture2DRectGray(tex1, vTexCoord0);


					vec2 x1 = vec2(uOffset.x, 0.);
					vec2 y1 = vec2(0., uOffset.y);

					//get the difference
					vec4 curdifX = b - a;
					vec4 curdifY = a - b; // the difference is opposite otherwise vy is flipped

					//calculate the gradient
					//for X________________
					vec4 gradx = texture2DRectGray(tex1, vTexCoord0 + x1) - texture2DRectGray(tex1, vTexCoord0 - x1);
					gradx += texture2DRectGray(tex0, vTexCoord0 + x1) - texture2DRectGray(tex0, vTexCoord0 - x1);

					//for Y________________
					vec4 grady = texture2DRectGray(tex1, vTexCoord0 + y1) - texture2DRectGray(tex1, vTexCoord0 - y1);
					grady += texture2DRectGray(tex0, vTexCoord0 + y1) - texture2DRectGray(tex0, vTexCoord0 - y1) ;

					vec4 gradmag = sqrt((gradx * gradx) + (grady * grady) + vec4(uLambda));

					vec4 vx = curdifX * (gradx / gradmag);
					vec4 vy = curdifY * (grady / gradmag);
					oColor = getColorCoded(vx.r, vy.r, uScale);
				}
			))
		);
	
	}
	catch(const std::exception& e){
		// log shader errors
		ci::app::console() << e.what() << std::endl;
	}

	try {
		
		mBlurShader = ci::gl::GlslProg::create(ci::gl::GlslProg::Format()
			.vertex(CI_GLSL(150,
				uniform mat4 ciModelViewProjection;

				in vec4 ciPosition;
				in vec2 ciTexCoord0;

				out vec2 vTexCoord0;

				void main()
				{
					vTexCoord0 = ciTexCoord0;
					gl_Position = ciModelViewProjection * ciPosition;
				}
			))
			.fragment(CI_GLSL(150,
				uniform sampler2D uInTexture;
				uniform vec2 texOffset;
				in vec2 vTexCoord0;

				out vec4 oColor;

				uniform float uBlurSize;
				uniform float uHorizontalPass; // 0 or 1 to indicate vertical or horizontal pass
				uniform float uSigma;        // The uSigma value for the gaussian function: higher value means more blur
				uniform vec2 uTexOffset;
				// A good value for 9x9 is around 3 to 5
				// A good value for 7x7 is around 2.5 to 4
				// A good value for 5x5 is around 2 to 3.5

				const float pi = 3.14159265;

				vec4 get2DOff(sampler2D tex, vec2 coord) {
					vec4 col = texture(tex, coord);
					if (col.w > 0.95)  col.z = col.z * -1;
					return vec4(col.y - col.x, col.z, 1, 1);
				}


				vec4 getColorCoded(float x, float y, vec2 scale) {
					vec2 xout = vec2(max(x, 0.), abs(min(x, 0.))) * scale.x;
					vec2 yout = vec2(max(y, 0.), abs(min(y, 0.))) * scale.y;
					float dirY = 1;
					if (yout.x > yout.y)  dirY = 0.90;
					return vec4(xout.yx, max(yout.x, yout.y), dirY);
				}

				void main() {
					float numBlurPixelsPerSide = float(uBlurSize / 2);
					vec2 blurMultiplyVec = 0 < uHorizontalPass ? vec2(1.0, 0.0) : vec2(0.0, 1.0);

					// Incremental Gaussian Coefficent Calculation (See GPU Gems 3 pp. 877 - 889)
					vec3 incrementalGaussian;
					incrementalGaussian.x = 1.0 / (sqrt(2.0 * pi) * uSigma);
					incrementalGaussian.y = exp(-0.5 / (uSigma * uSigma));
					incrementalGaussian.z = incrementalGaussian.y * incrementalGaussian.y;

					vec4 avgValue = vec4(0.0, 0.0, 0.0, 0.0);
					float coefficientSum = 0.0;

					// Take the central sample first...
					avgValue += get2DOff(uInTexture, vTexCoord0.st) * incrementalGaussian.x;

					coefficientSum += incrementalGaussian.x;
					incrementalGaussian.xy *= incrementalGaussian.yz;

					// Go through the remaining 8 vertical samples (4 on each side of the center)

					for (float i = 1.0; i <= numBlurPixelsPerSide; i++) {
						avgValue += get2DOff(uInTexture, vTexCoord0.st - i * texOffset *
							blurMultiplyVec) * incrementalGaussian.x;
						avgValue += get2DOff(uInTexture, vTexCoord0.st + i * texOffset *
							blurMultiplyVec) * incrementalGaussian.x;
						coefficientSum += 2.0 * incrementalGaussian.x;
						incrementalGaussian.xy *= incrementalGaussian.yz;
					}

					vec4 finColor = avgValue / coefficientSum;

					oColor = getColorCoded(finColor.x, finColor.y, vec2(1, 1));
				}
			))
		);

	}
	catch (const std::exception & e) {
		// log shader errors
		ci::app::console() << e.what() << std::endl;
	}

	try {
		mReposShader = ci::gl::GlslProg::create(ci::gl::GlslProg::Format()
			.vertex(CI_GLSL(150,
				uniform mat4 ciModelViewProjection;

				in vec4 ciPosition;
				in vec2 ciTexCoord0;

				out vec2 vTexCoord0;

				void main()
				{
					vTexCoord0 = ciTexCoord0;
					gl_Position = ciModelViewProjection * ciPosition;
				}

			))
			.fragment(CI_GLSL(150,
				in vec2 vTexCoord0;
				uniform vec2 uAmount;
				uniform sampler2D  tex0;
				uniform sampler2D  tex1;

				out vec4 oColor;

				vec2 get2DOff(sampler2D tex, vec2 coord) {
					vec4 col = texture(tex, coord);
					if (col.w > 0.95)  col.z = col.z * -1;
					return vec2(-1 * (col.y - col.x), col.z);//,1,1);
				}

				void main()
				{
					vec2 coord = get2DOff(tex1, vTexCoord0) * uAmount + vTexCoord0;  //relative coordinates  
					vec4 repos = texture(tex0, coord);
					oColor = repos;
				}
			))
		);
	}
	catch (const std::exception& e) {
		// log shader errors
		ci::app::console() << e.what() << std::endl;
	}

}