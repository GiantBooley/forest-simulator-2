#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <AL/al.h>
#include <AL/alc.h>

#include <linmath.h>
#include <AudioFile.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <PerlinNoise.hpp>

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <filesystem>

#define PI 3.1415926535897932384626433832795028841971693993751058

using namespace std;

bool debug = false;

GLFWwindow* window;
bool windowIconified = false;

struct Vertex {
	float x, y, z;
	float u, v;
	float tileHealth;
	float lr, lg, lb;
};

float randFloat() {
	return static_cast<float> (rand()) / static_cast<float> (RAND_MAX);
}
float lerp(float a, float b, float t) {
	return (b - a) * t + a;
}

class Vec2 {
public:
	float x = 0.f;
	float y = 0.f;
	Vec2(float asdx, float asdy) {
		x = asdx;
		y = asdy;
	}
	Vec2() {
		Vec2(0.f, 0.f);
	}
};
struct iVec2 {
	int x, y;
};


int convertFileToOpenALFormat(AudioFile<float>* audioFile) {
	int bitDepth = audioFile->getBitDepth();
	if (bitDepth == 16) {
		return audioFile->isStereo() ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
	} else if (bitDepth == 8) {
		return audioFile->isStereo() ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
	} else {
		cerr << "ERROR: bad bit depth for audio file" << endl;
		return -1;
	}
};
class SoundDoerSound {
	public:
	AudioFile<float> monoSoundFile;
	vector<uint8_t> monoPCMDataBytes;
	bool isLooping = false;
	float volume = 1.f;

	SoundDoerSound(bool looping, float vlaume, string path) {
		isLooping = looping;
		volume = vlaume;
		if (!monoSoundFile.load(path)) {
			cerr << "ERROR: failed to load brain sound \"" << path << "\"" << endl;
		}
		monoSoundFile.writePCMToBuffer(monoPCMDataBytes);
	}
};
class SoundDoerBuffer {
	public:
	ALuint monoSoundBuffer;
	ALuint monoSource;
	SoundDoerBuffer(SoundDoerSound sound) {
		alGenBuffers(1, &monoSoundBuffer);
		alBufferData(monoSoundBuffer, convertFileToOpenALFormat(&sound.monoSoundFile), sound.monoPCMDataBytes.data(), sound.monoPCMDataBytes.size(), sound.monoSoundFile.getSampleRate());

		alGenSources(1, &monoSource);
		alSource3f(monoSource, AL_POSITION, 1.f, 0.f, 0.f);
		alSource3f(monoSource, AL_VELOCITY, 0.f, 0.f, 0.f);
		alSourcef(monoSource, AL_PITCH, sound.isLooping ? 1.f : (randFloat() * 0.2f + 0.9f));
		alSourcef(monoSource, AL_GAIN, sound.volume * 0.5f);
		alSourcei(monoSource, AL_LOOPING, sound.isLooping ? AL_TRUE : AL_FALSE);
		alSourcei(monoSource, AL_BUFFER, monoSoundBuffer);
	}
};
class SoundDoer {
	public:
	ALCdevice* device;
	ALCcontext* context;
	vector<SoundDoerSound> sounds = {
		//{false, 1.f, "resources/audio/brain.wav"},
	};
	vector<SoundDoerBuffer> buffers = {};
	SoundDoer() {
		const ALCchar* defaultDeviceString = alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
		device = alcOpenDevice(defaultDeviceString);
		if (!device) {
			cerr << "ERROR: failed loading default sound device" << endl;
		}
		cout << "INFO: openal device name: " << alcGetString(device, ALC_DEVICE_SPECIFIER) << endl;

		context = alcCreateContext(device, nullptr);

		if (!alcMakeContextCurrent(context)) {
			cerr << "ERROR: failed to make openal context current" << endl;
		}
		alListener3f(AL_POSITION, 0.f, 0.f, 0.f);
		alListener3f(AL_VELOCITY, 0.f, 0.f, 0.f);
		ALfloat forwardAndUpVectors[] = {
			1.f, 0.f, 0.f,
			0.f, 1.f, 0.f
		};
		alListenerfv(AL_ORIENTATION, forwardAndUpVectors);
	}
	void tickSounds() {
		for (int i = (int)buffers.size() - 1; i >= 0; i--) {
			ALint sourceState;
			alGetSourcei(buffers[i].monoSource, AL_SOURCE_STATE, &sourceState);
			if (sourceState != AL_PLAYING) {
				alDeleteSources(1, &buffers[i].monoSource);
				alDeleteBuffers(1, &buffers[i].monoSoundBuffer);
				buffers.erase(buffers.begin() + i);
			}
		}
	}
	void play(SoundDoerSound sound) {
		if (buffers.size() < 1000) {
			buffers.push_back({sound});
			alSourcePlay(buffers[(int)buffers.size() - 1].monoSource);
		}
	}
	void exit() {
		alcMakeContextCurrent(nullptr);
		alcDestroyContext(context);
		alcCloseDevice(device);
	}
};
SoundDoer soundDoer;

class Controls {
	public:
	   bool w = false;
	   bool a = false;
	   bool s = false;
	   bool d = false;
	   bool left = false;
	   bool right = false;
	   bool up = false;
	   bool down = false;
	   bool shift = false;
	   bool space = false;
	   bool mouseDown = false;
	   Vec2 mouse{0.f, 0.f};
	   Vec2 worldMouse{0.f, 0.f};
	   Vec2 clipMouse{0.f, 0.f};
	   Vec2 previousClipMouse{0.f, 0.f};
};
int width, height;
class Camera {
public:
	Vec2 pos{0.f, 0.f};
	float zoom = 13.f;
	
	float left() {return pos.x - 2.f * zoom;}
	float right() {return pos.x + 2.f * zoom;}
	float bottom() {return pos.y - 2.f * zoom * ((float)height / (float)width);}
	float top() {return pos.y + 2.f * zoom * ((float)height / (float)width);}
};
float distance(Vec2 v1, Vec2 v2) {
	return sqrt(powf(v2.x - v1.x, 2.f) + powf(v2.y - v1.y, 2.f));
}

string ftos(float f, int places) {
	stringstream stream;
	stream << fixed << setprecision(places) << f;
	return stream.str();
}
string commas(float f) {
	string it = ftos(f, 0);
	string otherit = "";
	for (int i = 0; i < (int)it.size(); i++) {
		otherit += it.at(i);
		if (((int)it.size() - i - 1) % 3 == 0 && (int)it.size() - i - 1 != 0) {
			otherit += ",";
		}
	}
	return otherit;
}
Controls controls;
float roundToPlace(float x, float place) {
	return round(x / place) * place;
}
unsigned int fps = 0U;
unsigned int fpsCounter = fps;
double lastFpsTime = 0.; // resets every second
double frameTime = 0.0f;
double lastFrameTime = 0.;
string getFileNameFromPath(string path) {
	for (int i = (int)path.size() - 1; i >= 0; i--) {
		if (i == 0 || path.at(i) == '/' || path.at(i) == '\\') {
			string extension = path.substr(i + 1);
			return extension.substr(0, extension.find('.'));
		}
	}
	return "idk";
}

Vec2 getCharacterCoords(char c) {
	Vec2 coords{31.f, 4.f};
	switch (c) {
		case 'a':coords = {0.f, 0.f};break;
		case 'b':coords = {1.f, 0.f};break;
		case 'c':coords = {2.f, 0.f};break;
		case 'd':coords = {3.f, 0.f};break;
		case 'e':coords = {4.f, 0.f};break;
		case 'f':coords = {5.f, 0.f};break;
		case 'g':coords = {6.f, 0.f};break;
		case 'h':coords = {7.f, 0.f};break;
		case 'i':coords = {8.f, 0.f};break;
		case 'j':coords = {9.f, 0.f};break;
		case 'k':coords = {10.f, 0.f};break;
		case 'l':coords = {11.f, 0.f};break;
		case 'm':coords = {12.f, 0.f};break;
		case 'n':coords = {13.f, 0.f};break;
		case 'o':coords = {14.f, 0.f};break;
		case 'p':coords = {15.f, 0.f};break;
		case 'q':coords = {16.f, 0.f};break;
		case 'r':coords = {17.f, 0.f};break;
		case 's':coords = {18.f, 0.f};break;
		case 't':coords = {19.f, 0.f};break;
		case 'u':coords = {20.f, 0.f};break;
		case 'v':coords = {21.f, 0.f};break;
		case 'w':coords = {22.f, 0.f};break;
		case 'x':coords = {23.f, 0.f};break;
		case 'y':coords = {24.f, 0.f};break;
		case 'z':coords = {25.f, 0.f};break;

		case '1':coords = {0.f, 2.f};break;
		case '2':coords = {1.f, 2.f};break;
		case '3':coords = {2.f, 2.f};break;
		case '4':coords = {3.f, 2.f};break;
		case '5':coords = {4.f, 2.f};break;
		case '6':coords = {5.f, 2.f};break;
		case '7':coords = {6.f, 2.f};break;
		case '8':coords = {7.f, 2.f};break;
		case '9':coords = {8.f, 2.f};break;
		case '0':coords = {9.f, 2.f};break;
		
		case 'A':coords = {0.f, 1.f};break;
		case 'B':coords = {1.f, 1.f};break;
		case 'C':coords = {2.f, 1.f};break;
		case 'D':coords = {3.f, 1.f};break;
		case 'E':coords = {4.f, 1.f};break;
		case 'F':coords = {5.f, 1.f};break;
		case 'G':coords = {6.f, 1.f};break;
		case 'H':coords = {7.f, 1.f};break;
		case 'I':coords = {8.f, 1.f};break;
		case 'J':coords = {9.f, 1.f};break;
		case 'K':coords = {10.f, 1.f};break;
		case 'L':coords = {11.f, 1.f};break;
		case 'M':coords = {12.f, 1.f};break;
		case 'N':coords = {13.f, 1.f};break;
		case 'O':coords = {14.f, 1.f};break;
		case 'P':coords = {15.f, 1.f};break;
		case 'Q':coords = {16.f, 1.f};break;
		case 'R':coords = {17.f, 1.f};break;
		case 'S':coords = {18.f, 1.f};break;
		case 'T':coords = {19.f, 1.f};break;
		case 'U':coords = {20.f, 1.f};break;
		case 'V':coords = {21.f, 1.f};break;
		case 'W':coords = {22.f, 1.f};break;
		case 'X':coords = {23.f, 1.f};break;
		case 'Y':coords = {24.f, 1.f};break;
		case 'Z':coords = {25.f, 1.f};break;
		
		case '!':coords = {26.f, 0.f};break;
		case '@':coords = {27.f, 0.f};break;
		case '#':coords = {28.f, 0.f};break;
		case '$':coords = {29.f, 0.f};break;
		case '%':coords = {30.f, 0.f};break;
		case '^':coords = {31.f, 0.f};break;
		case '&':coords = {26.f, 1.f};break;
		case '*':coords = {27.f, 1.f};break;
		case '(':coords = {28.f, 1.f};break;
		case ')':coords = {29.f, 1.f};break;
		case '-':coords = {30.f, 1.f};break;
		case '=':coords = {31.f, 1.f};break;
		
		case '`':coords = {10.f, 2.f};break;
		case '~':coords = {11.f, 2.f};break;
		case '_':coords = {12.f, 2.f};break;
		case '+':coords = {13.f, 2.f};break;
		case '[':coords = {14.f, 2.f};break;
		case ']':coords = {15.f, 2.f};break;
		case '{':coords = {16.f, 2.f};break;
		case '}':coords = {17.f, 2.f};break;
		case '\\':coords = {18.f, 2.f};break;
		case '|':coords = {19.f, 2.f};break;
		case ';':coords = {20.f, 2.f};break;
		case '\'':coords = {21.f, 2.f};break;
		case ':':coords = {22.f, 2.f};break;
		case '"':coords = {23.f, 2.f};break;
		case ',':coords = {24.f, 2.f};break;
		case '.':coords = {25.f, 2.f};break;
		case '<':coords = {26.f, 2.f};break;
		case '>':coords = {27.f, 2.f};break;
		case '/':coords = {28.f, 2.f};break;
		case '?':coords = {29.f, 2.f};break;
		case ' ':coords = {30.f, 2.f};break;
	};
	return coords;
};
string getFileText(string path) {
	ifstream file{path};
	if (!file.is_open()) {
		return "";
		cerr << "ERROR: file \"" << path << "\" not found" << endl;
		file.close();
	}
	string text = "";
	string lineText;
	while (file) {
		if (!file.good()) {
			break;
		}
		getline(file, lineText);
		text += lineText + "\n";
	}
	file.close();
	return text;
}
class Shader {
	public:
	GLuint shader;
	Shader(string fileName, int shaderType) {

		string shaderText = getFileText(fileName);
		
		const char* text = shaderText.c_str();
		shader = glCreateShader(shaderType);
		glShaderSource(shader, 1, &text, NULL);
		glCompileShader(shader);

		// erors
		GLint success = GL_FALSE;  
		GLint infoLogLength;
		char infoLog[512];
		glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		if (infoLogLength > 0) {
			cerr << "ERROR: " << infoLog << endl;
		}
		if (!success){
			glDeleteShader(shader);
		} else cout << "INFO: Successfuly loaded " << (shaderType == GL_VERTEX_SHADER ? "vertex shader" : "fragment shader") << " \"" << fileName << "\"" << endl;
	}
};
class Material {
	public:
		GLint mvp_location, texture1_location, vpos_location, vtexcoord_location, vtilehealth_location, vlightcolor_location, time_location;
		GLuint program;

		int textureWidth, textureHeight, textureColorChannels;
		unsigned char* textureBytes;
		unsigned int texture;
		string name = "";

		Material(string namea, GLuint vertexShader, GLuint fragmentShader, string textureFile) {
			name = namea;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			
			textureBytes = stbi_load(textureFile.c_str(), &textureWidth, &textureHeight, &textureColorChannels, 0);
			if (textureBytes) {
				glTexImage2D(GL_TEXTURE_2D, 0, textureColorChannels == 4 ? GL_RGBA : GL_RGB, textureWidth, textureHeight, 0, textureColorChannels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, textureBytes);
				glGenerateMipmap(GL_TEXTURE_2D);
				cout << "INFO: successfully loaded texture file \"" << textureFile << "\"" << endl;
			} else {
				cerr << "ERROR: failed to load texture file \"" << textureFile << "\"" << endl;
			};
			stbi_image_free(textureBytes);

			program = glCreateProgram();
		
			glAttachShader(program, vertexShader);
			glAttachShader(program, fragmentShader);
			glLinkProgram(program);
			
			mvp_location = glGetUniformLocation(program, "MVP");
			texture1_location = glGetUniformLocation(program, "texture1");
			time_location = glGetUniformLocation(program, "time");
			vpos_location = glGetAttribLocation(program, "vPos");
			vtexcoord_location = glGetAttribLocation(program, "vTexCoord");
			vtilehealth_location = glGetAttribLocation(program, "vTileHealth");
			vlightcolor_location = glGetAttribLocation(program, "vLightColor");
		}
};
int newEntityID = 0;
class Item {
	public:
	int type;
	float durability;
	float damage;
	string material;
	Vec2 size;

	float attackDelay;
	float swingRotation = 0.f;
	float isSwinging = false;

	Item(int typea) {
		type = typea;
		switch (type) {
			case 0:
			durability = 600.f;
			damage = 3.f;
			material = "sword";
			size = {0.3f, 1.f};
		}
	}
};
class Entity {
public:
	Vec2 pos = {20.f, 720.f};
	Vec2 size = {0.5f, 1.8f};
	Vec2 vel = {0.f, 0.f};
	int id = newEntityID++;
	bool onGround = false;
	string material;
	int type;
	int controlsType; // 0-wasd,1-follow near player
	float health;
	float maxHealth;
	float speed = 1.6f;
	Item hand{0};
	iVec2 facingVector = {0, 0};
	float punchDelay = 0.5f;
	float punchDelayTimer = 0.f;
	
	Entity(int typea) {
		type = typea;
		material = type == 0 ? "player" : "sentry";
		controlsType = type == 0 ? 0 : 1;
		health = type == 0 ? 10123123123.f : 20.f;
		maxHealth = health;
	}
};
struct EntityCollision {
	bool collided;
	bool isEntityCollision;
	Entity* entity1;
	Entity* entity2;
};
namespace ttypes {
	enum types {
		air,dirt,stone,wood,log,leaves
	};
}
class Tile {
	public:
	int type;
	float health = 1.f;
	float maxHealth;
	Tile(int atype) {
		type = atype;
		switch (type) {
			case (ttypes::air):health = 43289.f;break;
			case (ttypes::dirt):health = 3.f;break;
			case (ttypes::stone):health = 60.f;break;
			case (ttypes::wood):health = 7.f;break;
			case (ttypes::log):health = 10.f;break;
			case (ttypes::leaves):health = 0.5f;break;
		}
		maxHealth = health;
	}
	Tile() {
		Tile((int)ttypes::air);
	}
};
class PerlinGenerator {
	public:
		static const int MAX_VERTICES = 1024;
		float r[MAX_VERTICES];
		PerlinGenerator() {
			for (int i = 0; i < MAX_VERTICES; i++) {
				r[i] = randFloat();
			}
		}
		float getVal(float x) {
			float t = x - floor(x);
			float tRemapSmoothstep = powf(t, 2.f) * (3.f - 2.f * t);
			int xMin = (int)x & (MAX_VERTICES - 1);
			int xMax = (xMin + 1) & (MAX_VERTICES - 1);
			return lerp(r[xMin], r[xMax], tRemapSmoothstep);
		}
};
class Particle {
	public:
	Vec2 pos{0.f, 0.f};
	Vec2 vel{0.f, 0.f};
	float time;
	string material;
	Particle(Vec2 poss, Vec2 vels, float times, string matial) {
		pos = poss;
		vel = vels;
		time = times;
		material = matial;
	}
};
struct Vec3 {
	float r, g, b;
};
struct Light {
	Vec2 pos;
	Vec3 intensity;
};
struct Photon {
	float x, y, xv, yv;
	float r, g, b;
};
// jeffrey thombpson blog line rect intsersection
bool lineLine(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4) {

  // calculate the direction of the lines
  float uA = ((x4-x3)*(y1-y3) - (y4-y3)*(x1-x3)) / ((y4-y3)*(x2-x1) - (x4-x3)*(y2-y1));
  float uB = ((x2-x1)*(y1-y3) - (y2-y1)*(x1-x3)) / ((y4-y3)*(x2-x1) - (x4-x3)*(y2-y1));

  // if uA and uB are between 0-1, lines are colliding
  if (uA >= 0 && uA <= 1 && uB >= 0 && uB <= 1) {
    //float intersectionX = x1 + (uA * (x2-x1));
    //float intersectionY = y1 + (uA * (y2-y1));
    return true;
  }
  return false;
}
bool lineRect(float x1, float y1, float x2, float y2, float rx, float ry, float rw, float rh) {
  bool left =   lineLine(x1,y1,x2,y2, rx,ry,rx, ry+rh);
  bool right =  lineLine(x1,y1,x2,y2, rx+rw,ry, rx+rw,ry+rh);
  bool top =    lineLine(x1,y1,x2,y2, rx,ry, rx+rw,ry);
  bool bottom = lineLine(x1,y1,x2,y2, rx,ry+rh, rx+rw,ry+rh);

  return (left || right || top || bottom);
}
void rotateVector(float* x, float* y, float theta) {
	float oldX = *x;
	*x = *x * cos(theta) - *y * sin(theta);
	*y = *y * cos(theta) + oldX * sin(theta);
}
class World {
	public:
	vector<Entity> entities = {{0}};
	vector<Particle> particles = {};
	Camera camera;
	const static int worldWidth = 5000;
	const static int worldHeight = 1000;

	const siv::PerlinNoise::seed_type seed = 123456u;
	const siv::PerlinNoise perlin{ seed };
	const siv::PerlinNoise::seed_type seed2 = 123457u;
	const siv::PerlinNoise perlin2{ seed2 };

	// lighting vars
	Tile tiles[worldWidth][worldHeight];
	Tile bgTiles[worldWidth][worldHeight];
	Vec3 lightmap[worldWidth][worldHeight];
	int lightmapN[worldWidth][worldHeight];
	vector<Photon> photons = {};

	PerlinGenerator gen;
	float time = 0.f;
	float getGeneratorHeight(float x) {
		float height = 0.f;
		for (int i = 0; i < 7; i++) {
			float it = powf(2.f, (float)i);
			height += gen.getVal(x * 0.1f * it) / it * 10.f;
		}
		return height + 700.f;
	}
	World() {
		// level generation generate generator generating generated generates generatings generations generational generationality generationalities
		for (int x = 0; x < worldWidth; x++) {
			float height = getGeneratorHeight(x);
			for (int y = 0; y < worldHeight; y++) {
				lightmap[x][y].r = 1.f;
				lightmap[x][y].g = 1.f;
				lightmap[x][y].b = 1.f;
				tiles[x][y] = {y < height ? (y < height - 10.f ? ttypes::stone : ttypes::dirt) : ttypes::air};
				bgTiles[x][y] = {y < height ? (y < height - 10.f ? ttypes::stone : ttypes::dirt) : ttypes::air};
				if (perlin.octave2D_01((double)x / 10., (double)y / 10., 4) < 0.5f || perlin2.octave2D_01((double)x / 10., (double)y / 10., 4) < 0.5f) tiles[x][y] = {ttypes::air};
			}
		}
		// tree generation
		for (int x = 2; x < worldWidth - 2; x++) {
			for (int y = worldHeight - 1 - 10; y >= 1; y--) {
				if (tiles[x][y].type == ttypes::air && tiles[x][y - 1].type == ttypes::dirt && randFloat() < 0.05f) {
					tiles[x][y] = ttypes::log;
					tiles[x][y + 1] = {ttypes::log};
					tiles[x][y + 2] = {ttypes::log};
					tiles[x][y + 3] = {ttypes::log};
					tiles[x][y + 4] = {ttypes::log};
					tiles[x][y + 5] = {ttypes::log};
					tiles[x][y + 6] = {ttypes::leaves};
					tiles[x][y + 7] = {ttypes::leaves};
					tiles[x][y + 8] = {ttypes::leaves};
					tiles[x - 1][y + 6] = {ttypes::leaves};
					tiles[x - 1][y + 7] = {ttypes::leaves};
					tiles[x - 1][y + 8] = {ttypes::leaves};
					tiles[x + 1][y + 6] = {ttypes::leaves};
					tiles[x + 1][y + 7] = {ttypes::leaves};
					tiles[x + 1][y + 8] = {ttypes::leaves};
					
					tiles[x - 2][y + 6] = {ttypes::leaves};
					tiles[x - 2][y + 7] = {ttypes::leaves};
					tiles[x + 2][y + 6] = {ttypes::leaves};
					tiles[x + 2][y + 7] = {ttypes::leaves};
				}
			}
		}
	}
	Vec3 currentLightmap[worldWidth][worldHeight];
	void lightingStep(vector<Light> lights, float dt) {
		return;//asdasd
		float photonSpeed = 0.5f;
		float howmanyper = 0.08f;
		for (float r = 0; r < PI * 2.f; r += howmanyper) {
			photons.push_back({
				lights[0].pos.x,
				lights[0].pos.y,
				sin(r) * photonSpeed,
				cos(r) * photonSpeed,
				lights[0].intensity.r, 
				lights[0].intensity.g, 
				lights[0].intensity.b
			});
		}
		int right = camera.right();
		int top = camera.top();
		for (int x = max((int)camera.left(), 0); x < min(right, static_cast<int>(worldWidth)); x++) {
			photons.push_back({
				(float)x + 0.5f,
				min(camera.top(), (float)(static_cast<int>(worldHeight))) - 0.5f,
				0.f,
				-1.f,
				0.5f,
				0.5f,
				0.5f
			});
			for (int y = max((int)camera.bottom(), 0); y < min(top, static_cast<int>(worldHeight)); y++) {
				currentLightmap[x][y] = {0, 0, 0};
			}
		}
		for (int i = (int)photons.size() - 1; i >= 0; i--) {
			for (int j = 0; j < 15; j++) { // rt steps
				if (isPointAir(photons[i].x + photons[i].xv, photons[i].y)) photons[i].x += photons[i].xv;
				else { // diffuse
					currentLightmap[(int)(photons[i].x + photons[i].xv)][(int)photons[i].yv].r += photons[i].r;
					currentLightmap[(int)(photons[i].x + photons[i].xv)][(int)photons[i].yv].g += photons[i].g;
					currentLightmap[(int)(photons[i].x + photons[i].xv)][(int)photons[i].yv].b += photons[i].b;
					photons[i].xv *= -1.f;
					rotateVector(&photons[i].xv, &photons[i].yv, randFloat() * 3.14f - 1.57f);
					photons[i].r -= 0.1f;
					photons[i].g -= 0.1f;
					photons[i].b -= 0.1f;
				}
				if (isPointAir(photons[i].x, photons[i].y + photons[i].yv)) photons[i].y += photons[i].yv;
				else {
					currentLightmap[(int)photons[i].x][(int)(photons[i].y + photons[i].yv)].r += photons[i].r;
					currentLightmap[(int)photons[i].x][(int)(photons[i].y + photons[i].yv)].g += photons[i].g;
					currentLightmap[(int)photons[i].x][(int)(photons[i].y + photons[i].yv)].b += photons[i].b;
					photons[i].yv *= -1.f;
					rotateVector(&photons[i].xv, &photons[i].yv, randFloat() * 3.14f - 1.57f);
					photons[i].r -= 0.1f;
					photons[i].g -= 0.1f;
					photons[i].b -= 0.1f;
				}

				currentLightmap[(int)photons[i].x][(int)photons[i].y].r += photons[i].r;
				currentLightmap[(int)photons[i].x][(int)photons[i].y].g += photons[i].g;
				currentLightmap[(int)photons[i].x][(int)photons[i].y].b += photons[i].b; // if not aabb check line
				if (
					(photons[i].r + photons[i].g + photons[i].b) / 3.f <= 0.f ||
					(
						(
							photons[i].x > camera.right() ||
							photons[i].x < camera.left() ||
							photons[i].y < camera.bottom() ||
							photons[i].y > camera.top()
						) &&
						!lineRect(
							photons[i].x,
							photons[i].y,
							photons[i].x + photons[i].xv * 32767.f,
							photons[i].y + photons[i].yv * 32767.f,
							camera.left(),
							camera.bottom(),
							camera.right() - camera.left(),
							camera.top() - camera.bottom()
						)
					)
				) {
					photons.erase(photons.begin() + i);
					break;
				}
			}
		}
		for (int x = max((int)camera.left(), 0); x < min(right, static_cast<int>(worldWidth)); x++) {
			for (int y = max((int)camera.bottom(), 0); y < min(top, static_cast<int>(worldHeight)); y++) {
				if (lightmapN[x][y] > 0) {
					lightmap[x][y].r = (lightmap[x][y].r * (float)lightmapN[x][y] + currentLightmap[x][y].r) / (float)(lightmapN[x][y] + 1);
					lightmap[x][y].g = (lightmap[x][y].g * (float)lightmapN[x][y] + currentLightmap[x][y].g) / (float)(lightmapN[x][y] + 1);
					lightmap[x][y].b = (lightmap[x][y].b * (float)lightmapN[x][y] + currentLightmap[x][y].b) / (float)(lightmapN[x][y] + 1);
				} else {
					lightmap[x][y].r = currentLightmap[x][y].r;
					lightmap[x][y].g = currentLightmap[x][y].g;
					lightmap[x][y].b = currentLightmap[x][y].b;
				}
				lightmapN[x][y]++;
				lightmapN[x][y] = min(lightmapN[x][y], 200);
			}
		}
	}
	Tile getTile(int x, int y) {
		if (x < 0 || y < 0 || x >= worldWidth || y >= worldHeight) return {0};
		return tiles[x][y];
	}
	Tile getBgTile(int x, int y) {
		if (x < 0 || y < 0 || x >= worldWidth || y >= worldHeight) return {0};
		return bgTiles[x][y];
	}
	bool isPointAir(float x, float y) {
		if (x < 0 || y < 0 || x >= worldWidth || y >= worldHeight) return false;
		return tiles[(int)x][(int)y].type == ttypes::air;
	}
	void setTile(int x, int y, Tile t) {
		if (x < 0 || y < 0 || x >= worldWidth || y >= worldHeight) return;
		tiles[x][y] = t;
	}
	void damageTile(int x, int y, float amount) {
		if (x < 0 || y < 0 || x >= worldWidth || y >= worldHeight) return;
		tiles[x][y].health -= amount;
		if (tiles[x][y].health <= 0.f) {
			tiles[x][y].type = ttypes::air;
			tiles[x][y].health = 43289.f;
		}
	}
	bool areTwoEntitiesCollidingWithEachother(Entity* e1, Entity* e2) {
		if (e1->id == e2->id) return false;
		return e1->pos.x + e1->size.x / 2.f > e2->pos.x - e2->size.x / 2.f && e1->pos.x - e1->size.x / 2.f < e2->pos.x + e2->size.x / 2.f && e1->pos.y + e1->size.y > e2->pos.y && e1->pos.y < e2->pos.y + e2->size.y;
	}
	EntityCollision getEntityCollision(Entity* e) {
		if (getTile((int)(e->pos.x - e->size.x / 2.f), (int)e->pos.y).type != ttypes::air || getTile((int)(e->pos.x + e->size.x / 2.f), (int)e->pos.y).type != ttypes::air || getTile((int)(e->pos.x - e->size.x / 2.f), (int)(e->pos.y + e->size.y)).type != ttypes::air || getTile((int)(e->pos.x + e->size.x / 2.f), (int)(e->pos.y + e->size.y)).type != ttypes::air) return {true, false, e, nullptr};
		for (int i = 0; i < (int)entities.size(); i++) {
			if (entities[i].id == e->id) continue;
			if (areTwoEntitiesCollidingWithEachother(e, &entities[i])) return {true, true, e, &entities[1]};
		}
		return {false, false, e, nullptr};
	}
	void makeEntityPunch(Entity* e) {
		if (e->punchDelayTimer > 0.f) return;
		e->punchDelayTimer = e->punchDelay;
		e->hand.swingRotation = 0.f;
		e->hand.isSwinging = true;
		if (e->facingVector.x != -1) {
			if (getTile((int)e->pos.x + 1, (int)e->pos.y + 1).type != ttypes::air) {
				damageTile((int)e->pos.x + 1, (int)e->pos.y + 1, 0.3f);
			} else {
				damageTile((int)e->pos.x + 1, (int)e->pos.y, 0.3f);
			}
		} else {
			if (getTile((int)e->pos.x - 1, (int)e->pos.y + 1).type != ttypes::air) {
				damageTile((int)e->pos.x - 1, (int)e->pos.y + 1, 0.3f);
			} else {
				damageTile((int)e->pos.x - 1, (int)e->pos.y, 0.3f);
			}
		}
	}
};
float lerpd(float a, float b, float t, float d) {
	return (b - a) * (t + d * 0.f) + a;
}
class GameState {
	public:
	World world;
	float dt = 1.f;
	float x = 0.f;
	float playing = true;
	void tick() {
		controls.previousClipMouse = controls.clipMouse;
		controls.clipMouse.x = (controls.mouse.x / (float)width - 0.5f) * 2.f;
		controls.clipMouse.y = (0.5f - controls.mouse.y / (float)height) * 2.f;
		controls.worldMouse.x = controls.clipMouse.x * (world.camera.zoom * (float)width / (float)height) + world.camera.pos.x;
		controls.worldMouse.y = controls.clipMouse.y * world.camera.zoom + world.camera.pos.y;
		// Physics Tracing Extreme
		float gravity = -9.807f;

		world.lightingStep({{{world.entities[0].pos.x, world.entities[0].pos.y + world.entities[0].size.y / 2.f}, {0.15f, 0.07f, 0.07f}}}, dt);
		if (controls.mouseDown && controls.worldMouse.x > 0.f && controls.worldMouse.x < world.worldWidth && controls.worldMouse.y > 0.f && controls.worldMouse.y < world.worldHeight) {
			world.damageTile((int)controls.worldMouse.x, (int)controls.worldMouse.y, dt);
		}

		if (controls.space) {
			Entity e = {1};
			e.pos.x = randFloat() * 100.f;
			if (!world.getEntityCollision(&e).collided) world.entities.push_back(e);
		}
		//find player is
		vector<int> playerIs;
		for (int i = 0; i < (int)world.entities.size(); i++) {
			if (world.entities[i].type == 0) playerIs.push_back(i);
		}
		for (int i = (int)world.entities.size() - 1; i >= 0; i--) {
			Entity* e = &world.entities[i];
			if (randFloat() < 0.0002f && e->type != 0) {
				Entity latest = {e->type};
				if (!world.getEntityCollision(&latest).collided) {
					latest.pos.y = e->pos.y + e->size.y;
					latest.pos.x = e->pos.x;
					latest.speed += 0.3f * (randFloat() - 0.5f);
					world.entities.push_back(latest);
				}
			}
			e->punchDelayTimer -= dt;
			if (e->punchDelayTimer < 0.f) e->punchDelayTimer = 0.f;

			bool controlsLeft = false;
			bool controlsRight = false;
			bool controlsUp = false;
			if (e->controlsType == 0) {
				controlsLeft = controls.a || controls.left;
				controlsRight = controls.d || controls.right;
				controlsUp = controls.w || controls.up;
			} else {
				int nearestPlayerI = -1;
				float nearestPlayerDistance = 0.f;
				for (int j = 0; j < (int)playerIs.size(); j++) {
					float currentDistance = distance(world.entities[playerIs[j]].pos, e->pos);
					if ((nearestPlayerI == -1 || currentDistance < nearestPlayerDistance) && world.entities[playerIs[j]].id != e->id) {
						nearestPlayerDistance = currentDistance;
						nearestPlayerI = j;
					}
				}
				if (nearestPlayerI != -1) {
					controlsUp = true;
					controlsLeft = (world.entities[playerIs[nearestPlayerI]].pos.x < e->pos.x);
					controlsRight = (world.entities[playerIs[nearestPlayerI]].pos.x > e->pos.x);
					world.makeEntityPunch(e);
				}
			}
			if (e->hand.isSwinging) {
				e->hand.swingRotation += 5.f * dt;
				if (e->hand.swingRotation > PI) {
					e->hand.swingRotation = 0.f;
					e->hand.isSwinging = false;
				}
			};

			float friction = e->onGround ? 0.9f : 0.01f;
			
			e->vel.y += gravity * dt;
			float netMovement = controlsRight - controlsLeft;
			if (netMovement != 0.f) {
				e->facingVector = {(int)netMovement, 0};
			}
			
			e->vel.x = lerpd(e->vel.x, netMovement * (controls.shift ? 6.f : e->speed), friction, dt);
			e->pos.x += e->vel.x * dt;

			// Collisions
			EntityCollision collision = world.getEntityCollision(e);
			if (collision.collided) {
				e->pos.x -= e->vel.x * dt;
				e->vel.x = 0.f;
			}
			e->pos.y += e->vel.y * dt;
			e->onGround = false;
			collision = world.getEntityCollision(e);
			if (collision.collided) {
				e->pos.y -= e->vel.y * dt;
				e->onGround = e->vel.y < 0.f;
				e->vel.y = 0.f;
				if (controlsUp && e->onGround) {
					e->vel.y = 6.f;
				}
			}
			if (e->type == 1) e->health -= 1.f * dt;
		}
		for (int i = (int)world.entities.size() - 1; i >= 0; i--) {
			if (world.entities[i].health <= 0.f) {
				world.particles.push_back({world.entities[i].pos, {0.f, 1.f}, 2.f, "skull"});
				world.entities.erase(world.entities.begin() + i);
			}
		}
		for (int i = (int)world.particles.size() - 1; i >= 0; i--) {
			world.particles[i].pos.x += world.particles[i].vel.x * dt;
			world.particles[i].pos.y += world.particles[i].vel.y * dt;
			world.particles[i].time -= dt;
			if (world.particles[i].time <= 0) world.particles.erase(world.particles.begin() + i);
		}

		if ((int)playerIs.size() > 0) {
			float targetX = 0.f;
			float targetY = 0.f;
			for (int i = 0; i < (int)playerIs.size(); i++) {
				targetX += world.entities[playerIs[i]].pos.x;
				targetY += world.entities[playerIs[i]].pos.y + world.entities[playerIs[i]].size.y / 2.f;
			}
			targetX /= (float)playerIs.size();
			targetY /= (float)playerIs.size();

			world.camera.pos.x = lerpd(world.camera.pos.x, targetX, 0.1f, 1.f);
			world.camera.pos.y = lerpd(world.camera.pos.y, targetY, 0.1f, 1.f);
		}
		
		world.time += dt;
	}
};
class GameStateRenderer {
public:
	GameState* game;

	float aspect;
	GLuint vertexBuffer,elementBuffer;

	Shader solidV{"resources/shader/solid.vsh", GL_VERTEX_SHADER};
	Shader guiV{"resources/shader/gui.vsh", GL_VERTEX_SHADER};
	Shader fontV{"resources/shader/font.vsh", GL_VERTEX_SHADER};

	Shader solidF{"resources/shader/solid.fsh", GL_FRAGMENT_SHADER};
	Shader guiF{"resources/shader/gui.fsh", GL_FRAGMENT_SHADER};

	vector<Material> materials = {
		{"sky"				      , solidV.shader , solidF.shader			, "resources/texture/sky.png"}, 
		{"dirt"					   , solidV.shader, solidF.shader		  , "resources/texture/dirt.png"},
		{"stone"					   , solidV.shader, solidF.shader		  , "resources/texture/stone.png"},
		{"wood"					   , solidV.shader, solidF.shader		  , "resources/texture/wood.png"},
		{"log"					   , solidV.shader, solidF.shader		  , "resources/texture/log.png"},
		{"leaves"					   , solidV.shader, solidF.shader		  , "resources/texture/leaves.png"},
		{"grass"					   , solidV.shader, solidF.shader		  , "resources/texture/grass.png"},
		{"grass_left"					   , solidV.shader, solidF.shader		  , "resources/texture/grass_left.png"},
		{"grass_right"					   , solidV.shader, solidF.shader		  , "resources/texture/grass_right.png"},
		{"tile_cracks"					   , solidV.shader, solidF.shader		  , "resources/texture/tile_cracks.png"},
		{"player"					  , solidV.shader, solidF.shader		  , "resources/texture/player.png"},
		{"sentry"					  , solidV.shader, solidF.shader		  , "resources/texture/sentry.png"},
		{"light_map"					   , solidV.shader, solidF.shader		  , "resources/texture/light_map.png"},
		{"select"					   , solidV.shader, solidF.shader		  , "resources/texture/select.png"},
		{"skull"					   , solidV.shader, solidF.shader		  , "resources/texture/skull.png"},
		{"sword"					   , solidV.shader, solidF.shader		  , "resources/texture/sword.png"},

		{"gui_font"				      , fontV.shader , guiF.shader			, "resources/texture/font.png"}, 
	};

	vector<Vertex> vertices = {};
	vector<vector<unsigned int>> indiceses = {};

	int getMatID(string name) {
		int size = (int)materials.size();
		for (int i = 0; i < size; i++) {
			if (name == materials[i].name) return i;
		}
		return 0;
	}
	GameStateRenderer(GameState* gam) {
		game = gam;
		
		// create index buffers
		for (int i = 0; i < (int)materials.size(); i++) {
			indiceses.push_back({});
		}

		// vertex buffer
		glGenBuffers(1, &vertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

		// index buffer
		glGenBuffers(1, &elementBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
	}
	void buildThem() {
		aspect = (float)width / (float)height;
		clearVertices();
		addRect(-20000.f, -10000.f, -10000.f, 40000.f, 20000.f, getMatID("sky"));
		for (int x = 0; x < game->world.worldWidth; x++) {
			if ((float)x + 1.f < game->world.camera.left()) continue;
			if ((float)x > game->world.camera.right()) break;
			for (int y = 0; y < game->world.worldHeight; y++) {
				if ((float)y + 1.f < game->world.camera.bottom()) continue;
				if ((float)y > game->world.camera.top()) break;
				float lightR = game->world.lightmap[x][y].r;
				float lightG = game->world.lightmap[x][y].g;
				float lightB = game->world.lightmap[x][y].b;
				addTile(x, y, 0, game->world.tiles[x][y].type, lightR, lightG, lightB);
				addTile(x, y, -5, game->world.bgTiles[x][y].type, lightR, lightG, lightB);
				if (game->world.tiles[x][y].health < game->world.tiles[x][y].maxHealth) {
					addRect((float)x, (float)y, 0.f, 1.f, 1.f, getMatID("tile_cracks"), floor((game->world.tiles[x][y].maxHealth - game->world.tiles[x][y].health) / game->world.tiles[x][y].maxHealth * 8.f) / 8.f, 0.f, 1.f / 8.f, 1.f);
				}
			}
		}
		addRect(floor(controls.worldMouse.x), floor(controls.worldMouse.y), 0.f, 1.f, 1.f, getMatID("select"));
		for (int i = 0; i < (int)game->world.entities.size(); i++) {
			Entity* e = &game->world.entities[i];
			addRect(e->pos.x - e->size.x / 2.f, e->pos.y, 0.f, e->size.x, e->size.y, getMatID(e->material));
			float handX = (game->world.entities[i].facingVector.x == -1) ? (e->pos.x - e->size.x / 2.f - 0.2f) : (e->pos.x + e->size.x / 2.f + 0.1f);
			addRotatedRect(handX, e->pos.y + e->size.y / 2.f + 0.1f, 0.f, e->hand.size.x, e->hand.size.y, getMatID(e->hand.material), e->hand.swingRotation * ((e->facingVector.x < 0) ? -1.f : 1.f), handX, e->pos.y + e->size.y / 2.f + 0.1f);
		}
		for (int i = 0; i < (int)game->world.particles.size(); i++) {
			addRect(game->world.particles[i].pos.x - 0.5f, game->world.particles[i].pos.y - 0.5f, 0.f, 1.f, 1.f, getMatID(game->world.particles[i].material));
		}
		addText("fps: " + to_string(fps), -0.9f, 0.9f, -0.1f, 0.05f, 0.8f, 2.f, false);
		int tris = 0;
		for (int i = 0; i < (int)indiceses.size(); i++) {
			tris += (int)indiceses[i].size() / 3;
		}
		addText("tris: " + to_string(tris), -0.9f, 0.1f, -0.1f, 0.05f, 0.8f, 2.f, false);
		addText("photons: " + to_string(game->world.photons.size()), -0.9f, -0.05f, -0.1f, 0.05f, 0.8f, 2.f, false);
		addText("block size: " + to_string((float)height / game->world.camera.zoom / 2.f) + "px", -0.9f, -0.2f, -0.1f, 0.05f, 0.8f, 2.f, false);
	}
	void renderMaterials() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, width, height);
		buildThem();
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), &vertices[0], GL_STATIC_DRAW);
		for (int i = 0; i < (int)materials.size(); i++) {
			if ((int)indiceses[i].size() > 0) renderMaterial(i);
		}

	}
	void renderMaterial(int id) {


		glEnableVertexAttribArray(materials[id].vpos_location);
		glVertexAttribPointer(materials[id].vpos_location, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (void*)0);
	
		glEnableVertexAttribArray(materials[id].vtexcoord_location);
		glVertexAttribPointer(materials[id].vtexcoord_location, 2, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (void*)(sizeof(float) * 3));
	
		glEnableVertexAttribArray(materials[id].vtilehealth_location);
		glVertexAttribPointer(materials[id].vtilehealth_location, 1, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (void*)(sizeof(float) * 5));
	
		glEnableVertexAttribArray(materials[id].vlightcolor_location);
		glVertexAttribPointer(materials[id].vlightcolor_location, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (void*)(sizeof(float) * 6));

		float ratio = (float)width / (float)height;
		mat4x4 m, p, mvp;

		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indiceses[id].size() * sizeof(unsigned int), &indiceses[id][0], GL_STATIC_DRAW);
		
		glBindTexture(GL_TEXTURE_2D, materials[id].texture);

		mat4x4_identity(m);
		//mat4x4_scale_aniso(m, m, ratio, 1.f, 1.f);
		mat4x4_translate(m, -game->world.camera.pos.x, -game->world.camera.pos.y, -game->world.camera.zoom);
		/*mat4x4_rotate_X(m, m, game->world.camera.rotation.x);
		mat4x4_rotate_Y(m, m, game->world.camera.rotation.y);
		mat4x4_rotate_Z(m, m, game->world.camera.rotation.z);*/
		//mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
		mat4x4_perspective(p, 1.57f, ratio, 0.1f, 20000.f);
		mat4x4_mul(mvp, p, m);

		glUseProgram(materials[id].program);

		glUniform1i(materials[id].texture1_location, 0);
		glUniform1f(materials[id].time_location, game->world.time);
		glUniformMatrix4fv(materials[id].mvp_location, 1, GL_FALSE, (const GLfloat*)mvp);
		glDrawElements(GL_TRIANGLES, indiceses[id].size(), GL_UNSIGNED_INT, (void*)0);
	}
private:
	void clearVertices() {
		vertices.clear();
		for (int i = 0; i < (int)indiceses.size(); i++) {
			indiceses[i].clear();
		}
	}

	//============================
	//== world space =============
	//===========================

	void addPlane(float x, float y, float z, float w, float d, bool worldUv, int matId) {
		unsigned int end = vertices.size();
		indiceses[matId].insert(indiceses[matId].end(), {
			0U+end, 1U+end, 2U+end,
			1U+end, 3U+end, 2U+end
		});
		vertices.insert(vertices.end(), {
			{x  , y , z+d, 0.f              , 0.f              , 1.f, 1.f, 1.f, 1.f},
			{x+w, y , z+d, worldUv ? w : 1.f, 0.f              , 1.f, 1.f, 1.f, 1.f},
			{x  , y , z  , 0.f              , worldUv ? d : 1.f, 1.f, 1.f, 1.f, 1.f},
			{x+w, y , z  , worldUv ? w : 1.f, worldUv ? d : 1.f, 1.f, 1.f, 1.f, 1.f}
		});
	}
	void addQuad(Vec2 p1, Vec2 p2, Vec2 p3, Vec2 p4, int matId) { // clockwise
		unsigned int end = vertices.size();
		indiceses[matId].insert(indiceses[matId].end(), {
			0U+end, 1U+end, 2U+end,
			0U+end, 2U+end, 3U+end
		});
		vertices.insert(vertices.end(), {
			{p1.x, p1.y, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f},
			{p2.x, p2.y, 0.f, 1.f, 0.f, 1.f, 1.f, 1.f, 1.f},
			{p3.x, p3.y, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f, 1.f},
			{p4.x, p4.y, 0.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f}
		});
	}

	//============================
	//== clip space ==============
	//============================

	void addRect(float x, float y, float z, float w, float h, int matId, float u = 0.f, float v = 0.f, float s = 1.f, float t = 1.f) {
		if (x + w < game->world.camera.left() || x > game->world.camera.right() || y + h < game->world.camera.bottom() || y > game->world.camera.top()) return;
		unsigned int end = vertices.size();
		indiceses[matId].insert(indiceses[matId].end(), {
			0U+end, 2U+end, 1U+end,
			1U+end, 2U+end, 3U+end
		});
		vertices.insert(vertices.end(), {
			{x  , y+h, z, u    , v    , 1.f, 1.f, 1.f, 1.f},
			{x+w, y+h, z, u + s, v    , 1.f, 1.f, 1.f, 1.f},
			{x  , y  , z, u    , v + t, 1.f, 1.f, 1.f, 1.f},
			{x+w, y  , z, u + s, v + t, 1.f, 1.f, 1.f, 1.f}
		});
	}
	void addWorldRect(float x, float y, float z, float w, float h, int matId, float tileHealth, float lightR, float lightG, float lightB) {
		if (x + w < game->world.camera.left() || x > game->world.camera.right() || y + h < game->world.camera.bottom() || y > game->world.camera.top()) return;
		unsigned int end = vertices.size();
		indiceses[matId].insert(indiceses[matId].end(), {
			0U+end, 2U+end, 1U+end,
			1U+end, 2U+end, 3U+end
		});
		vertices.insert(vertices.end(), {
			{x  , y+h, z, 0.f, 0.f, tileHealth, lightR, lightG, lightB},
			{x+w, y+h, z, 1.f, 0.f, tileHealth, lightR, lightG, lightB},
			{x  , y  , z, 0.f, 1.f, tileHealth, lightR, lightG, lightB},
			{x+w, y  , z, 1.f, 1.f, tileHealth, lightR, lightG, lightB}
		});
	}
	void addTile(int x, int y, int z, int type, float lightR, float lightG, float lightB) {
		switch (type) {
			case ttypes::air:
			//addWorldRect((float)x, (float)y, (float)z, 1.f, 1.f, getMatID("air"), 1.f, lightR, lightG, lightB);
			break;
			case 1:
			addWorldRect((float)x, (float)y, (float)z, 1.f, 1.f, getMatID("dirt"), 1.f, lightR, lightG, lightB);
			if (game->world.getTile(x, y + 1).type == ttypes::air) {
				addWorldRect((float)x, (float)y, (float)z, 1.f, 1.f, getMatID("grass"), 1.f, lightR, lightG, lightB);
			} else {
				if (game->world.getTile(x + 1, y + 1).type == ttypes::air && game->world.getTile(x + 1, y).type == ttypes::dirt) {
					addWorldRect((float)x, (float)y, (float)z, 1.f, 1.f, getMatID("grass_left"), 1.f, lightR, lightG, lightB);
				}
				if (game->world.getTile(x - 1, y + 1).type == ttypes::air && game->world.getTile(x - 1, y).type == ttypes::dirt) {
					addWorldRect((float)x, (float)y, (float)z, 1.f, 1.f, getMatID("grass_right"), 1.f, lightR, lightG, lightB);
				}
			}
			break;
			case 2:
			addWorldRect((float)x, (float)y, (float)z, 1.f, 1.f, getMatID("stone"), 1.f, lightR, lightG, lightB);
			break;
			case 3:
			addWorldRect((float)x, (float)y, (float)z, 1.f, 1.f, getMatID("wood"), 1.f, lightR, lightG, lightB);
			break;
			case 4:
			addWorldRect((float)x, (float)y, (float)z, 1.f, 1.f, getMatID("log"), 1.f, lightR, lightG, lightB);
			break;
			case 5:
			addWorldRect((float)x, (float)y, (float)z, 1.f, 1.f, getMatID("leaves"), 1.f, lightR, lightG, lightB);
			break;
		}
	}
	void addRotatedRect(float x, float y, float z, float w, float h, int matId, float theta, float originX, float originY) {
		if (x + w < game->world.camera.left() || x > game->world.camera.right() || y + h < game->world.camera.bottom() || y > game->world.camera.top()) return;
		unsigned int end = vertices.size();
		indiceses[matId].insert(indiceses[matId].end(), {
			0U+end, 2U+end, 1U+end,
			1U+end, 2U+end, 3U+end
		});
		theta *= -1;
		Vec2 rectVerts[4] = {{x, y + h}, {x + w, y + h}, {x, y}, {x + w, y}};
		for (int i = 0; i < 4; i++) {
			rectVerts[i].x -= originX;
			rectVerts[i].y -= originY;
			float px = rectVerts[i].x;
			float py = rectVerts[i].y;
			rectVerts[i].x = px * cos(theta) - py * sin(theta);
			rectVerts[i].y = py * cos(theta) + px * sin(theta);
			rectVerts[i].x += originX;
			rectVerts[i].y += originY;
		}
		vertices.insert(vertices.end(), {
			{rectVerts[0].x, rectVerts[0].y, z, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f},
			{rectVerts[1].x, rectVerts[1].y, z, 1.f, 0.f, 1.f, 1.f, 1.f, 1.f},
			{rectVerts[2].x, rectVerts[2].y, z, 0.f, 1.f, 1.f, 1.f, 1.f, 1.f},
			{rectVerts[3].x, rectVerts[3].y, z, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f}
		});
	};
	void addCharacter(char character, float x, float y, float z, float w) {
		Vec2 uv = getCharacterCoords(character);
		unsigned int end = vertices.size();
		indiceses[getMatID("gui_font")].insert(indiceses[getMatID("gui_font")].end(), {
			0U+end, 2U+end, 1U+end,
			1U+end, 2U+end, 3U+end
		});
		vertices.insert(vertices.end(), {
			{x  , y+w*aspect, z, 0.f+uv.x, 0.f+uv.y, 1.f, 1.f, 1.f, 1.f},
			{x+w, y+w*aspect, z, 1.f+uv.x, 0.f+uv.y, 1.f, 1.f, 1.f, 1.f},
			{x  , y         , z, 0.f+uv.x, 1.f+uv.y, 1.f, 1.f, 1.f, 1.f},
			{x+w, y         , z, 1.f+uv.x, 1.f+uv.y, 1.f, 1.f, 1.f, 1.f}
		});
	}
	void addText(string text, float x, float y, float z, float w, float spacing, float maxWidth, bool centered) {
		for (int i = 0; i < (int)text.length(); i++) {
			float noWrapX = (float)i * w * spacing;
			float wrapX = fmod(noWrapX, maxWidth);
			if (centered) wrapX -= maxWidth / 4.f;
			addCharacter(text.at(i), wrapX + x, y - floor(noWrapX / maxWidth) * w * 1.6f, z, w * 1.5f);
			z -= 0.001;
		}
	}
};
GameState game;
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_ESCAPE) {
			game.playing = !game.playing;
		} else
		if (key == GLFW_KEY_Q) {
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}
		else if (key == GLFW_KEY_W) controls.w = true;
		else if (key == GLFW_KEY_A) controls.a = true;
		else if (key == GLFW_KEY_S) controls.s = true;
		else if (key == GLFW_KEY_D) controls.d = true;
		else if (key == GLFW_KEY_F) game.world.entities[0].vel.y += 20.f;
		else if (key == GLFW_KEY_LEFT_SHIFT) controls.shift = true;
		else if (key == GLFW_KEY_UP) controls.up = true;
		else if (key == GLFW_KEY_DOWN) controls.down = true;
		else if (key == GLFW_KEY_LEFT) controls.left = true;
		else if (key == GLFW_KEY_RIGHT) controls.right = true;
		else if (key == GLFW_KEY_SPACE) {
			controls.space = true;
		}
		else if (key == GLFW_KEY_X) game.world.makeEntityPunch(&game.world.entities[0]);
	}
	else if (action == GLFW_RELEASE) {
		if (key == GLFW_KEY_W) controls.w = false;
		else if (key == GLFW_KEY_A) controls.a = false;
		else if (key == GLFW_KEY_S) controls.s = false;
		else if (key == GLFW_KEY_D) controls.d = false;
		else if (key == GLFW_KEY_LEFT_SHIFT) controls.shift = false;
		else if (key == GLFW_KEY_UP) controls.up = false;
		else if (key == GLFW_KEY_DOWN) controls.down = false;
		else if (key == GLFW_KEY_LEFT) controls.left = false;
		else if (key == GLFW_KEY_RIGHT) controls.right = false;
		else if (key == GLFW_KEY_SPACE) controls.space = false;
	}
}
bool mouseIntersectsClipRect(float x, float y, float w, float h) {
	return controls.clipMouse.x > x && controls.clipMouse.x < x + w && controls.clipMouse.y > y && controls.clipMouse.y < y + h;
}
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		double xpos, ypos;
		//getting cursor position
		glfwGetCursorPos(window, &xpos, &ypos);
		controls.mouseDown = true;
	}
	if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		double xpos, ypos;
		//getting cursor position
		glfwGetCursorPos(window, &xpos, &ypos);
		controls.mouseDown = false;
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		double xpos, ypos;
		//getting cursor position
		glfwGetCursorPos(window, &xpos, &ypos);
		game.world.setTile((int)controls.worldMouse.x, (int)controls.worldMouse.y, {ttypes::wood});
	}
}
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	controls.mouse = {(float)xpos, (float)ypos};
}
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	game.world.camera.zoom /= pow(1.1, yoffset);
}
static void iconify_callback(GLFWwindow* window, int iconified) {
	windowIconified = (iconified == GLFW_TRUE) ? true : false;
}


static void error_callback(int error, const char* description) {
	fprintf(stderr, "ERROR: %s\n", description);
}
int main(void) {
	glfwSetErrorCallback(error_callback);

	if (!glfwInit()) exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	window = glfwCreateWindow(640, 480, "Forest Simulator 2", NULL, NULL);
	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
		return 1;
	}

	GLFWimage images[1]{};
	images[0].pixels = stbi_load("icon.png", &images[0].width, &images[0].height, 0, 4); //rgba channels 
	glfwSetWindowIcon(window, 1, images);
	stbi_image_free(images[0].pixels);

	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetWindowIconifyCallback(window, iconify_callback);

	glfwMakeContextCurrent(window);
	gladLoadGL();
	glfwSwapInterval(1);

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwSetWindowSizeLimits(window, 160, 90, 160000, 90000);
	//glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE);
	//glFrontFace(GL_CW);
	//glCullFace(GL_FRONT);
	glEnable(GL_BLEND);
	glEnable(GL_MULTISAMPLE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glClearColor(0.4f, 0.4f, 0.9f, 1.f);
	lastFrameTime = glfwGetTime();
	
	GameStateRenderer renderer{&game};
	while (!glfwWindowShouldClose(window)) {
		if (game.playing && !windowIconified) {
			double newFrameTime = glfwGetTime();
			frameTime = newFrameTime - lastFrameTime;
			lastFrameTime = newFrameTime;
	
			fpsCounter++;
			if (glfwGetTime() - lastFpsTime > 1.) {
				lastFpsTime = glfwGetTime();
				fps = fpsCounter;
				fpsCounter = 0U;
			}
	
			glfwGetFramebufferSize(window, &width, &height);
			width = max(width, 1);
			height = max(height, 1);
	
			game.dt = min(0.5f * (float)frameTime, 0.5f);
			for (int i = 0; i < 2; i++) {
				game.tick();
			}
			renderer.renderMaterials();
			soundDoer.tickSounds();
	
			glfwSwapBuffers(window);
		}
		glfwPollEvents();
	}
	for (int i = 0; i < (int)renderer.materials.size(); i++) {
		glDeleteTextures(1, &renderer.materials[i].texture);
	}
	soundDoer.exit();
	glfwDestroyWindow(window);

	glfwTerminate();
	exit(EXIT_SUCCESS);
}