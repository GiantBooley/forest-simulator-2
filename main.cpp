#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <AL/al.h>
#include <AL/alc.h>

#include <linmath.h>
#include <AudioFile.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

using namespace std;

bool debug = false;

GLFWwindow* window;

struct Vertex {
	float x, y, z;
	float u, v;
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
};
bool pointIsInRectangleRange(Vec2 point, Vec2 radius) {
	return point.x < radius.x && point.x > -radius.x && point.y < radius.y && point.y > -radius.y;
}
bool lineCircleIntersects(float ax, float ay, float bx, float by, float cx, float cy, float r) {
	ax -= cx;
	ay -= cy;
	bx -= cx;
	by -= cy;
	float a = pow(bx - ax, 2.f) + pow(by - ay, 2.f);
	float b = 2.f*(ax*(bx - ax) + ay*(by - ay));
	float c = pow(ax, 2.f) + pow(ay, 2.f) - pow(r, 2.f);
	float disc = pow(b, 2.f) - 4.f*a*c;
	if(disc <= 0.f) return false;
	float sqrtdisc = sqrt(disc);
	float t1 = (-b + sqrtdisc)/(2.f*a);
	float t2 = (-b - sqrtdisc)/(2.f*a);
	if((0.f < t1 && t1 < 1.f) || (0.f < t2 && t2 < 1.f)) return true;
	return false;
}


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
	#define SOUND_ELECTRICITY 1
	#define SOUND_IRON 2
	#define SOUND_BUTTON 3
	#define SOUND_HURT 4
	#define SOUND_BOSS_DEATH 5
	#define SOUND_DEATH 6
	#define SOUND_PLACE 7
	#define SOUND_MONEY 8
	#define SOUND_MUSIC1 9
	#define SOUND_ROLL 10
	#define SOUND_ROBOT 11
	#define SOUND_MUSIC_HUMAN_PASSAGES 12
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
	   bool shift = false;
	   Vec2 mouse{0.f, 0.f};
	   Vec2 worldMouse{0.f, 0.f};
	   Vec2 clipMouse{0.f, 0.f};
	   Vec2 previousClipMouse{0.f, 0.f};
};
class Camera {
public:
	Vec2 pos{0.f, 0.f};
};

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
float frameTime = 0.0f;
unsigned int fps = 0U;
unsigned int fpsCounter = fps;
double lastFpsTime = 0U;
long long lastFrameTime = 0LL;
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
		GLint mvp_location, texture1_location, vpos_location, vtexcoord_location;
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
			vpos_location = glGetAttribLocation(program, "vPos");
			vtexcoord_location = glGetAttribLocation(program, "vTexCoord");
		}
};
class Entity {
public:
	Vec2 pos = {20.f, 30.f};
	Vec2 size = {0.5f, 1.8f};
	Vec2 vel = {0.f, 0.f};
	bool onGround = false;
};
class World {
	public:
	vector<Entity> entities = {{}};
	Camera camera;
	const static int worldWidth = 5000;
	const static int worldHeight = 100;
	int tiles[worldWidth][worldHeight];
	World() {
		for (int x = 0; x < worldWidth; x++) {
			int height = (int)(randFloat() * 4.f);
			for (int y = 0; y < worldHeight; y++) {
				tiles[x][y] = y < 20 + height ? (y < 15 ? 2 : 1) : 0;
			}
		}
	}
	int getTile(Vec2 pos) {
		if (pos.x < 0.f || pos.y < 0.f || pos.x >= worldWidth || pos.y >= worldHeight) return 0;
		return tiles[(int)pos.x][(int)pos.y];
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
	void tick(int width, int height) {
		controls.previousClipMouse = controls.clipMouse;
		controls.clipMouse.x = (controls.mouse.x / (float)width - 0.5f) * 2.f;
		controls.clipMouse.y = (0.5f - controls.mouse.y / (float)height) * 2.f;
		controls.worldMouse.x = controls.clipMouse.x * (10.f * (float)width / (float)height) + world.camera.pos.x;
		controls.worldMouse.y = controls.clipMouse.y * 10.f + world.camera.pos.y;

		// Physics Tracing Extreme
		float gravity = -9.807f;
		
		for (int i = 0; i < (int)world.entities.size(); i++) {
			Entity& e = world.entities[i];

			float friction = e.onGround ? 0.9f : 0.01f;
			
			e.vel.y += gravity * dt;
			float netMovement = controls.d - controls.a;
			e.vel.x = lerpd(e.vel.x, netMovement * (controls.shift ? 6.f : 1.6f), friction, dt);
			e.pos.x += e.vel.x * dt;

			// Collisions
			if (world.getTile({e.pos.x - e.size.x / 2.f, e.pos.y}) != 0 || world.getTile({e.pos.x + e.size.x / 2.f, e.pos.y}) != 0 || world.getTile({e.pos.x - e.size.x / 2.f, e.pos.y + e.size.y}) != 0 || world.getTile({e.pos.x + e.size.x / 2.f, e.pos.y + e.size.y}) != 0) {
				e.pos.x -= e.vel.x * dt;
				e.vel.x = 0.f;
			}
			e.pos.y += e.vel.y * dt;
			e.onGround = false;
			if (world.getTile({e.pos.x - e.size.x / 2.f, e.pos.y}) != 0 || world.getTile({e.pos.x + e.size.x / 2.f, e.pos.y}) != 0 || world.getTile({e.pos.x - e.size.x / 2.f, e.pos.y + e.size.y}) != 0 || world.getTile({e.pos.x + e.size.x / 2.f, e.pos.y + e.size.y}) != 0) {
				e.pos.y -= e.vel.y * dt;
				e.onGround = e.vel.y < 0.f;
				e.vel.y = 0.f;
				if (controls.w && e.onGround) {
					e.vel.y = 6.f;
				}
			}
		}
		world.camera.pos.x = lerpd(world.camera.pos.x, world.entities[0].pos.x, 0.1f, 1.f);
		world.camera.pos.y = lerpd(world.camera.pos.y, world.entities[0].pos.y, 0.1f, 1.f);
	}
};
float distance(Vec2 v1, Vec2 v2) {
	return sqrt(powf(v2.x - v1.x, 2.f) + powf(v2.y - v1.y, 2.f));
}
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
		{"solid"					  , solidV.shader, solidF.shader		  , "resources/texture/dirt.png"},
		{"dirt"					   , solidV.shader, solidF.shader		  , "resources/texture/dirt.png"},
		{"stone"					   , solidV.shader, solidF.shader		  , "resources/texture/stone.png"},
		{"select"					   , solidV.shader, solidF.shader		  , "resources/texture/select.png"},
		{"player"					  , solidV.shader, solidF.shader		  , "resources/texture/player.png"},
		{"gui_font"				   , fontV.shader , guiF.shader			, "resources/texture/font.png"}, 
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
	void buildThem(int width, int height) {
		aspect = (float)width / (float)height;
		clearVertices();

		for (int x = 0; x < game->world.worldWidth; x++) {
			for (int y = 0; y < game->world.worldHeight; y++) {
				if (pointIsInRectangleRange({(float)x - game->world.camera.pos.x, (float)y - game->world.camera.pos.y}, {20.f, 12.f})) {
					switch (game->world.tiles[x][y]) {
						case 1:
						addRect((float)x, (float)y, 0.f, 1.f, 1.f, getMatID(game->world.tiles[x][y + 1] == 0 ? "select" : "dirt"), {0.f, 0.f});
						break;
						case 2:
						addRect((float)x, (float)y, 0.f, 1.f, 1.f, getMatID("stone"), {0.f, 0.f});
						break;
					}
				}
			}
		}
		addRect(floor(controls.worldMouse.x), floor(controls.worldMouse.y), 0.f, 1.f, 1.f, getMatID("select"), {0.f, 0.f});
		for (int i = 0; i < (int)game->world.entities.size(); i++) {
			addRect(game->world.entities[i].pos.x - game->world.entities[i].size.x / 2.f, game->world.entities[i].pos.y, 0.f, game->world.entities[i].size.x, game->world.entities[i].size.y, getMatID("player"), {0.f, 0.f});
		}
		addText("fps: " + to_string(fps), -0.9f, 0.9f, -0.1f, 0.05f, 0.8f, 2.f, false);
		addText("y: " + to_string(game->world.entities[0].pos.y), -0.9f, 0.8f, -0.1f, 0.05f, 0.8f, 2.f, false);
	}
	void renderMaterials(int width, int height) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, width, height);

		buildThem(width, height);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), &vertices[0], GL_STATIC_DRAW);
		for (int i = 0; i < (int)materials.size(); i++) {
			if ((int)indiceses[i].size() > 0) renderMaterial(i, width, height);
		}

	}
	void renderMaterial(int id, int width, int height) {


		glEnableVertexAttribArray(materials[id].vpos_location);
		glVertexAttribPointer(materials[id].vpos_location, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (void*)0);
	
		glEnableVertexAttribArray(materials[id].vtexcoord_location);
		glVertexAttribPointer(materials[id].vtexcoord_location, 2, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (void*)(sizeof(float) * 3));
		float ratio = (float)width / (float)height;
		mat4x4 m, p, mvp;

		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indiceses[id].size() * sizeof(unsigned int), &indiceses[id][0], GL_STATIC_DRAW);
		
		glBindTexture(GL_TEXTURE_2D, materials[id].texture);

		mat4x4_identity(m);
		//mat4x4_scale_aniso(m, m, ratio, 1.f, 1.f);
		mat4x4_translate(m, -game->world.camera.pos.x, -game->world.camera.pos.y, -10.f);
		/*mat4x4_rotate_X(m, m, game->world.camera.rotation.x);
		mat4x4_rotate_Y(m, m, game->world.camera.rotation.y);
		mat4x4_rotate_Z(m, m, game->world.camera.rotation.z);*/
		//mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
		mat4x4_perspective(p, 1.57f, ratio, 0.1f, 200.f);
		mat4x4_mul(mvp, p, m);

		glUniform1i(materials[id].texture1_location, 0);
		glUseProgram(materials[id].program);
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
			{x  , y , z+d, 0.f, 0.f},
			{x+w, y , z+d, worldUv ? w : 1.f, 0.f},
			{x  , y , z  , 0.f, worldUv ? d : 1.f},
			{x+w, y , z  , worldUv ? w : 1.f, worldUv ? d : 1.f}
		});
	}
	void addQuad(Vec2 p1, Vec2 p2, Vec2 p3, Vec2 p4, int matId) { // clockwise
		unsigned int end = vertices.size();
		indiceses[matId].insert(indiceses[matId].end(), {
			0U+end, 1U+end, 2U+end,
			0U+end, 2U+end, 3U+end
		});
		vertices.insert(vertices.end(), {
			{p1.x, p1.y, 0.f, 0.f, 0.f},
			{p2.x, p2.y, 0.f, 1.f, 0.f},
			{p3.x, p3.y, 0.f, 0.f, 1.f},
			{p4.x, p4.y, 0.f, 1.f, 1.f}
		});
	}

	//============================
	//== clip space ==============
	//============================

	void addRect(float x, float y, float z, float w, float h, int matId, Vec2 uvOffset) {
		unsigned int end = vertices.size();
		indiceses[matId].insert(indiceses[matId].end(), {
			0U+end, 2U+end, 1U+end,
			1U+end, 2U+end, 3U+end
		});
		vertices.insert(vertices.end(), {
			{x  , y+h, z, 0.f + uvOffset.x, 0.f + uvOffset.y},
			{x+w, y+h, z, 1.f + uvOffset.x, 0.f + uvOffset.y},
			{x  , y  , z, 0.f + uvOffset.x, 1.f + uvOffset.y},
			{x+w, y  , z, 1.f + uvOffset.x, 1.f + uvOffset.y}
		});
	}
	void addRect(float x, float y, float w, float h, int matId) {
		unsigned int end = vertices.size();
		indiceses[matId].insert(indiceses[matId].end(), {
			0U+end, 2U+end, 1U+end,
			1U+end, 2U+end, 3U+end
		});
		vertices.insert(vertices.end(), {
			{x  , y+h, 0.f, 0.f, 0.f},
			{x+w, y+h, 0.f, 1.f, 0.f},
			{x  , y  , 0.f, 0.f, 1.f},
			{x+w, y  , 0.f, 1.f, 1.f}
		});
	}
	void addCharacter(char character, float x, float y, float z, float w) {
		Vec2 uv = getCharacterCoords(character);
		addRect(x, y, z, w, w * aspect, getMatID("gui_font"), uv);
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
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}
		else if (key == GLFW_KEY_W) controls.w = true;
		else if (key == GLFW_KEY_A) controls.a = true;
		else if (key == GLFW_KEY_S) controls.s = true;
		else if (key == GLFW_KEY_D) controls.d = true;
		else if (key == GLFW_KEY_LEFT_SHIFT) controls.shift = true;
		else if (key == GLFW_KEY_F) game.world.entities[0].vel.y = 20.f;
	}
	else if (action == GLFW_RELEASE) {
		if (key == GLFW_KEY_W) controls.w = false;
		else if (key == GLFW_KEY_A) controls.a = false;
		else if (key == GLFW_KEY_S) controls.s = false;
		else if (key == GLFW_KEY_D) controls.d = false;
		else if (key == GLFW_KEY_LEFT_SHIFT) controls.shift = false;
	};
}
bool mouseIntersectsClipRect(float x, float y, float w, float h) {
	return controls.clipMouse.x > x && controls.clipMouse.x < x + w && controls.clipMouse.y > y && controls.clipMouse.y < y + h;
}
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		double xpos, ypos;
		//getting cursor position
		glfwGetCursorPos(window, &xpos, &ypos);

		if (controls.worldMouse.x > 0.f && controls.worldMouse.x < game.world.worldWidth && controls.worldMouse.y > 0.f && controls.worldMouse.y < game.world.worldHeight) {
			game.world.tiles[(int)controls.worldMouse.x][(int)controls.worldMouse.y] = 1;
		}
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
	   double xpos, ypos;
	   //getting cursor position
	   glfwGetCursorPos(window, &xpos, &ypos);
	}
}
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	controls.mouse = {(float)xpos, (float)ypos};
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

	glfwMakeContextCurrent(window);
	gladLoadGL();
	glfwSwapInterval(1);

	glfwWindowHint(GLFW_SAMPLES, 4);
	//glEnable(GL_DEPTH_TEST);
	//glEnable(GL_CULL_FACE);
	//glFrontFace(GL_CW);
	//glCullFace(GL_FRONT);
	glEnable(GL_BLEND);
	glEnable(GL_MULTISAMPLE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glClearColor(0.5f, 0.5f, 0.9f, 1.f);
	lastFrameTime = (chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch())).count();
	
	GameStateRenderer renderer{&game};
	while (!glfwWindowShouldClose(window)) {
		long long newFrameTime = (chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch())).count();
		frameTime = (float)(newFrameTime - lastFrameTime) / 1000.f;
		lastFrameTime = newFrameTime;

		fpsCounter++;
		if (glfwGetTime() - lastFpsTime > 1.) {
			lastFpsTime = glfwGetTime();
			fps = fpsCounter;
			fpsCounter = 0U;
		}

		int width, height;
		
		glfwGetFramebufferSize(window, &width, &height);

		game.dt = 0.5f * frameTime;
		for (int i = 0; i < 2; i++) {
			game.tick(width, height);
		}

		renderer.renderMaterials(width, height);
		soundDoer.tickSounds();

		glfwSwapBuffers(window);
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