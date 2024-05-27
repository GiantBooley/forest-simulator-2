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
#include <random>
#include <cstdlib>

#define PI 3.1415926535897932384626433832795028841971693993751058

using namespace std;

bool debug = false;

GLFWwindow* window;
bool windowIconified = false;
unsigned int seed = 23456743;
unsigned int nextRandom(void) {
	unsigned int z = (seed += 0x9E3779B9u);
	z ^= z >> 16;
	z *= 0x21f0aaadu;
	z ^= z >> 15;
	z *= 0x735a2d97u;
	z ^= z >> 15;
	return z;
}
float randFloat() {
	return static_cast<float>(nextRandom()) / static_cast<float>(0xFFFFFFFF);
}
float lerp(float a, float b, float t) {
	return (b - a) * t + a;
}

int mod(int a, int b) {
    return (b + (a % b)) % b;
}
// jeffrey thombpson blog line rect intsersection
bool lineLine(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4) {

  // calculate the direction of the lines
  float uA = ((x4-x3)*(y1-y3) - (y4-y3)*(x1-x3)) / ((y4-y3)*(x2-x1) - (x4-x3)*(y2-y1));
  float uB = ((x2-x1)*(y1-y3) - (y2-y1)*(x1-x3)) / ((y4-y3)*(x2-x1) - (x4-x3)*(y2-y1));

  // fi ua nad ub aqe betwen 0d1 linxes aee coliding
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
struct Vec3 {
	float r, g, b;
};


int convertFileToOpenALFormat(AudioFile<float>* audioFile) {
	int bitDepth = audioFile->getBitDepth();
	if (bitDepth == 16) {
		return audioFile->isStereo() ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
	} else if (bitDepth == 8) {
		return audioFile->isStereo() ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8;
	} else {
		cerr << "[ERROR] bad bit depth for audio file" << endl;
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
			cerr << "[ERROR] failed to load brain sound \"" << path << "\"" << endl;
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
			cerr << "[ERROR] failed loading default sound device" << endl;
		}
		cout << "[INFO] openal device name: " << alcGetString(device, ALC_DEVICE_SPECIFIER) << endl;

		context = alcCreateContext(device, nullptr);

		if (!alcMakeContextCurrent(context)) {
			cerr << "[ERROR] failed to make openal context current" << endl;
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
			alGetSourcei(buffers.at(i).monoSource, AL_SOURCE_STATE, &sourceState);
			if (sourceState != AL_PLAYING) {
				alDeleteSources(1, &buffers.at(i).monoSource);
				alDeleteBuffers(1, &buffers.at(i).monoSoundBuffer);
				buffers.erase(buffers.begin() + i);
			}
		}
	}
	void play(SoundDoerSound sound) {
		if (buffers.size() < 1000) {
			buffers.push_back({sound});
			alSourcePlay(buffers.at((int)buffers.size() - 1).monoSource);
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
	   bool f = false;
	   bool left = false;
	   bool right = false;
	   bool up = false;
	   bool down = false;
	   bool shift = false;
	   bool space = false;
	   bool mouseDown = false;
	   bool xPressed = false;
	   bool bPressed = false;
	   bool e = false;
	   Vec2 mouse{0.f, 0.f};
	   Vec2 worldMouse{0.f, 0.f};
	   Vec2 clipMouse{0.f, 0.f};
	   Vec2 previousClipMouse{0.f, 0.f};
};
int width, height;
class Camera {
public:
	Vec2 pos{0.f, 0.f};
	float zoomTarget = 7.f;
	float zoom = zoomTarget;
	
	float left(float z) {
		return pos.x - 2.f * (zoom - z);
	}
	float right(float z) {
		return pos.x + 2.f * (zoom - z);
	}
	float bottom(float z) {
		return pos.y - 2.f * (zoom - z) * ((float)height / (float)width);
	}
	float top(float z) {
		return pos.y + 2.f * (zoom - z) * ((float)height / (float)width);
	}
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
double frameTime = 0.;
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

string getFileText(string path) {
	ifstream file{path};
	if (!file.is_open()) {
		return ""; // asdasd fix do fix
		cerr << "[ERROR] file \"" << path << "\" not found" << endl;
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
			cerr << "[ERROR] " << infoLog << endl;
		}
		if (!success){
			glDeleteShader(shader);
		} else cout << "[INFO] Loaded " << (shaderType == GL_VERTEX_SHADER ? "vertex shader" : "fragment shader") << " \"" << fileName << "\"" << endl;
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
				cout << "[INFO] Loaded texture \"" << textureFile << "\"" << endl;
			} else {
				cerr << "[ERROR] Failed loading texture \"" << textureFile << "\"" << endl;
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
namespace itypes {
	enum itypes {
		none, sword, excalibur, burger, lantern, pickaxe, antbread, dirt, snow, stone, wood, log, leaves
	};
}
namespace etypes {
	enum etypes {
		player, sentry, mimic, merchant, item
	};
}
namespace ttypes {
	enum types {
		none,air,dirt,snow,stone,wood,log,leaves,sentry_shack_bottom,sentry_shack_middle,sentry_shack_top
	};
}
class Item {
	public:
	int type;
	float durability;
	float maxDurability;
	float damage;
	string material;
	Vec2 size;
	bool isEdible;
	float eatingHealth = 3.f;
	Vec3 emission{0.f, 0.f, 0.f};

	float punchDelay = 0.5f;

	Item(int typea) {
		type = typea;
		switch (type) {
			case itypes::none:
			durability = 29843298670.f;
			damage = 0.f;
			material = "empty";
			size = {0.3f, 1.f};
			punchDelay = 0.5f;
			break;

			case itypes::sword:
						break;

			case itypes::excalibur:
						break;

			case itypes::burger:
					break;

			case itypes::lantern:
					break;

			case itypes::pickaxe:
						break;

			case itypes::antbread:
						break;

			/* ======================= */
				//BLOCKs
			/* ========================*/

			case itypes::snow:
			
			break;

			case itypes::stone:
						break;

			case itypes::dirt:
						break;

			case itypes::wood:
			
			break;

			case itypes::log:
						break;
		}
		maxDurability = durability;
	}
};
class ItemSword : public Item {
	ItemSword() : Item() {
		durability = 70.f;
		damage = 1.5f;
		material = "sword";
		size = {0.3f, 1.f};
		punchDelay = 0.5f;
	}
}
class ItemExcalibur : public Item {
	ItemExcalibur() : Item() {
		durability = 100.f;
		damage = 1000.f;
		material = "excalibur";
		size = {1.f, 1.5f};
		punchDelay = 0.12f;
		emission = {0.1f, 0.02f, 0.02f};
	}
}
class ItemBurger : public Item {
	ItemBurger() : Item() {
		durability = 5.f;
		damage = 0.1f;
		material = "burger";
		size = {1.f, 1.f};
		punchDelay = 1.f;
		isEdible = true;
		eatingHealth = 3.f;
	}
}
class ItemLantern : public Item {
	ItemLantern() : Item() {
		durability = 5.f;
		damage = 0.1f;
		material = "lantern";
		size = {1.f, 1.f};
		punchDelay = 1.f;
		emission = {0.3f, 0.15f, 0.15f};

	}
}
class ItemPickaxe : public Item {
	ItemPickaxe() : Item() {
		durability = 250.f;
		damage = 20.f;
		material = "pickaxe";
		size = {1.f, 1.f};
		punchDelay = 1.25f;

	}
}
class ItemAntbread : public Item {
	ItemAntbread() : Item() {
		durability = 3.f;
		damage = 0.1f;
		material = "antbread";
		size = {1.f, 1.f};
		punchDelay = 1.f;
		isEdible = true;
		eatingHealth = 1.5f;

	}
}
class ItemSnow : public Item {
	ItemSnow() : Item() {
		durability = 25.f;
		damage = 0.5f;
		material = "snow";
		size = {1.f, 1.f};
		punchDelay = 1.f;
	}
}
class ItemStone : public Item {
	ItemStone() : Item() {
		durability = 25.f;
		damage = 0.5f;
		material = "stone";
		size = {1.f, 1.f};
		punchDelay = 1.f;
	}
}
class ItemDirt : public Item {
	ItemDirt() : Item() {
		durability = 25.f;
		damage = 0.5f;
		material = "dirt";
		size = {1.f, 1.f};
		punchDelay = 1.f;

	}
}
class ItemWood : public Item {
	ItemWood() : Item() {
		durability = 25.f;
		damage = 0.5f;
		material = "wood";
		size = {1.f, 1.f};
		punchDelay = 1.f;
	}
}
class ItemLog : public Item {
	ItemLog() : Item() {
		durability = 25.f;
		damage = 0.5f;
		material = "log";
		size = {1.f, 1.f};
		punchDelay = 1.f;
	}
}
class SkinList {
	public:
	vector<string> list;
	SkinList() {
		for (const auto & entry : filesystem::directory_iterator("resources/texture/skins")) {
			list.push_back(getFileNameFromPath(entry.path().u8string()));
		}
	}
};
SkinList skins;
class Entity {
public:
	Vec2 pos = {20.f, 100.f};
	Vec2 size = {0.5f, 1.8f};
	Vec2 vel = {0.f, 0.f};
	int id = newEntityID++;
	bool onGround = false;
	string material;
	int type;
	int controlsType; // 0-wasd,1-follow near player,2-mimic movement,3-none
	float health;
	bool isDamagable = true;
	float maxHealth;
	float speed = 1.6f;
	vector<Item> items;
	int itemNumber = 0;
	iVec2 facingVector = {0, 0};
	float punchDelayTimer = 0.f;
	float swingRotation = 0.f;
	float isSwinging = false;
	bool canPickUpStuff = false;
	float damageRedness = 0.f;
	float particleEmitTimer = 0.f;
	float particleEmitDelay = 0.1f;
	float age = 0.f;
	
	Entity(string maaterial, int caontrolsType, Vec2 saize, float haealth, bool caanPickUpStuff) {
		for (int i = 0; i < 8; i++) {
			items.push_back({itypes::none});
		}
		material = maaterial;
		controlsType = caontrolsType;
		size = saize;
		health = haealth;
		canPickUpStuff = caanPickUpStuff;

		maxHealth = health;
	}
};
class EntityPlayer : public Entity {
	public:
		EntityPlayer() : Entity("skin_" + skins.list.at(1), 0, {0.5f, 1.8f}, 10.f, true) {
			items.at(0) = {itypes::sword};
			items.at(1) = {itypes::excalibur};
			items.at(2) = {itypes::burger};
			items.at(3) = {itypes::lantern};
			items.at(4) = {itypes::pickaxe};
			items.at(5) = {itypes::antbread};
			items.at(6) = {itypes::snow};
			type = etypes::player;
		}
};
class EntitySentry : public Entity {
	public:
		EntitySentry() : Entity("sentry", 1, {0.6f, 2.8f}, 10.f, false) {
			items.at(0) = {itypes::sword};
			type = etypes::sentry;
		}
};
class EntityMimic : public Entity {
	public:
		EntityMimic() : Entity("mimic", 2, {0.6f, 2.8f}, 10.f, false) {
			items.at(0) = {itypes::sword};
			type = etypes::mimic;
		}
};
class EntityMerchant : public Entity {
	public:
		vector<Item> stock;
		EntityMerchant() : Entity("merchant", 1, {0.5f, 1.8f}, 10.f, false) {
			items.at(0) = {itypes::sword};
			type = etypes::merchant;
			for (int i = 0; i < 10; i++) {
				stock.push_back({itypes::dirt});
			}
		}
};
class EntityItem : public Entity {
	public:
		EntityItem() : Entity("empty", 3, {0.5f, 0.5f}, 10123.f, false) {
			isDamagable = false;
			type = etypes::item;
		}
};

struct EntityCollision {
	bool collided;
	vector<iVec2> tileCollisions;
	bool isEntityCollision;
	Entity* entity1;
	Entity* entity2;
};
class Tile {
	public:
	int type;
	float health = 1.f;
	float maxHealth;
	float friction = 0.5f;
	bool isSolid = false;
	Item drops{itypes::none};
	Tile(int atype) {
		type = atype;
		switch (type) {
			case ttypes::none:
				health = 11111111.f;
				friction = 0.f;
				isSolid = false;
				break;
			case ttypes::air:
				health = 43289.f;
				friction = 0.f;
				isSolid = false;
				break;
			case ttypes::dirt:
				health = 3.f;
				friction = 0.95f;
				isSolid = true;
				drops = {itypes::dirt};
				break;
			case ttypes::snow:
				health = 1.5f;
				friction = 0.8f;
				isSolid = true;
				break;
			case ttypes::stone:
				health = 60.f;
				friction = 0.95f;
				isSolid = true;
				drops = {itypes::stone};
				break;
			case ttypes::wood:
				health = 7.f;
				friction = 0.9f;
				isSolid = true;
				drops = {itypes::wood};
				break;
			case ttypes::log:
				health = 10.f;
				friction = 0.9f;
				isSolid = true;
				drops = {itypes::log};
				break;
			case ttypes::leaves:
				health = 0.5f;
				friction = 0.8f;
				isSolid = true;
				drops = {itypes::leaves};
				break;
			case ttypes::sentry_shack_bottom:
				health = 13.f;
				friction = 0.1f;
				isSolid = false;
				break;
			case ttypes::sentry_shack_middle:
				health = 13.f;
				friction = 0.1f;
				isSolid = false;
				break;
			case ttypes::sentry_shack_top:
				health = 13.f;
				friction = 0.1f;
				isSolid = false;
				break;
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
				r[i] = randFloat() * 2.f - 1.f;
			}
		}
		float getVal(float x) { // xmin okgoookgfgfoookgfo
			int xMin = mod(floor(x), static_cast<float>(MAX_VERTICES));
			int xMax = mod(floor(x) + 1.f, static_cast<float>(MAX_VERTICES));
			return lerp(r[xMin], r[xMax], smoothstep(x - floor(x)));
		}
	private:
		float smoothstep(float x) {
			return x * x * (3.f - 2.f * x);
		}
};
class Particle {
	public:
	Vec2 pos{0.f, 0.f};
	Vec2 vel{0.f, 0.f};
	Vec2 size{1.f, 1.f};
	float time;
	string material;
	Particle(Vec2 poss, Vec2 vels, Vec2 sizes, float times, string matial) {
		pos = poss;
		vel = vels;
		size = sizes;
		time = times;
		material = matial;
	}
};
struct Light {
	Vec2 pos;
	Vec3 intensity;
};
struct Photon {
	float x, y, xv, yv;
	float r, g, b;
};
static const int SECT_SIZE = 128;
class TileSection {
public:
	int x;
	int y;
	Tile tiles[SECT_SIZE][SECT_SIZE];
	Tile bgTiles[SECT_SIZE][SECT_SIZE];
	Vec3 lightmap[SECT_SIZE][SECT_SIZE];
	Vec3 currentLightmap[SECT_SIZE][SECT_SIZE];
	int lightmapN[SECT_SIZE][SECT_SIZE];
	TileSection(int ax, int ay) {
		x = ax;
		y = ay;
	}
	void clearCurrentLightmap() {
		for (int x = 0; x < SECT_SIZE; x++) {
			for (int y = 0; y < SECT_SIZE; y++) {
				currentLightmap[x][y] = {0.f, 0.f, 0.f};
			}
		}
	}
	void solveLightmap() {
		for (int x = 0; x < SECT_SIZE; x++) {
			for (int y = 0; y < SECT_SIZE; y++) {
				if (lightmapN[x][y] > 0) {
					lightmap[x][y].r = (lightmap[x][y].r * (float)lightmapN[x][y] + currentLightmap[x][y].r) / (float)(lightmapN[x][y] + 1);
					lightmap[x][y].g = (lightmap[x][y].g * (float)lightmapN[x][y] + currentLightmap[x][y].g) / (float)(lightmapN[x][y] + 1);
					lightmap[x][y].b = (lightmap[x][y].b * (float)lightmapN[x][y] + currentLightmap[x][y].b) / (float)(lightmapN[x][y] + 1);
				} else {
					lightmap[x][y].r = currentLightmap[x][y].r;
					lightmap[x][y].g = currentLightmap[x][y].g;
					lightmap[x][y].b = currentLightmap[x][y].b;
				}
				lightmapN[x][y] = min(lightmapN[x][y] + 1, 200);
			}
		}
	}
};
struct TileAddress {
	int i, x, y;
};
struct QueueTile {
	Tile tile;
	int x, y;
};
class World {
	public:
	vector<Entity*> entities;
	int self = -1;
	vector<Particle> particles = {};
	Camera camera;

	const siv::PerlinNoise::seed_type seed = 123456u;
	const siv::PerlinNoise perlin{ seed };
	const siv::PerlinNoise::seed_type seed2 = 123457u;
	const siv::PerlinNoise perlin2{ seed };

	vector<TileSection> sections;
	vector<Photon> photons = {};

	vector<Vec3> clouds;

	static const int octaves = 10;
	PerlinGenerator gen[octaves];
	TileSection gs{0, 0}; // generating section
	float time = 0.f;
	World() {
		for (int i = 0; i < 500; i++) {
			clouds.push_back({randFloat() * 10000.f, randFloat() * 10.f + 20.f, randFloat() * -3000.f - 10.f});
		}
		Entity* player = new EntityPlayer();
		player->pos.y = getGeneratorHeight(player->pos.x) + 5.f;
		spawnEntity(player);
	}
	float getGeneratorHeight(float x) {
		float height = 0.f;
		float scale = 200.f;
		for (int i = 0; i < octaves; i++) {
			float it = powf(2.f, (float)i);
			height += gen[i].getVal(x / scale * it) / it * scale * 0.25f;
		}
		return height;
	}
	Tile generateTile(int x, int y, float heightFake, bool hasHeight) {
		//return {y < 16};
		float height = hasHeight ? heightFake : getGeneratorHeight(x);
		int type = y < round(height) ? (y < round(height) - 10.f ? ttypes::stone : (y > 20 ? ttypes::snow : ttypes::dirt)) : ttypes::air;
		if (perlin.octave2D_01((double)x / 10., (double)y / 10., 2) < 0.25f || perlin2.octave2D_01((double)x / 10., (double)y / 10., 2) < 0.1f) type = ttypes::air;
		return {type};
	}
	void setLocalGSTile(int x, int y, Tile tile) {
		if (x >= 0 && y >= 0 && x < SECT_SIZE && y < SECT_SIZE) {
			gs.tiles[x][y] = tile;
			return;
		}
		for (int i = 0; i < (int)sections.size(); i++) {
			if (sections.at(i).x == (int)floor((float)x / static_cast<float>(SECT_SIZE)) && sections.at(i).y == (int)floor((float)y / static_cast<float>(SECT_SIZE))) {
				sections.at(i).tiles[mod(x, SECT_SIZE)][mod(y, SECT_SIZE)] = tile;
				return;
			}
		}
	}
	Tile getLocalGSTile(int x, int y) {
		if (x >= 0 && y >= 0 && x < SECT_SIZE && y < SECT_SIZE) {
			return gs.tiles[x][y];
		}
		for (int i = 0; i < (int)sections.size(); i++) {
			if (sections.at(i).x == (int)floor((float)x / static_cast<float>(SECT_SIZE)) && sections.at(i).y == (int)floor((float)y / static_cast<float>(SECT_SIZE))) {
				return sections.at(i).tiles[mod(x, SECT_SIZE)][mod(y, SECT_SIZE)];
			}
		}
		return {ttypes::air}; // do do replace
	}
	void setLocalGSBGTile(int x, int y, Tile tile) {
		if (x >= 0 && y >= 0 && x < SECT_SIZE && y < SECT_SIZE) {
			gs.bgTiles[x][y] = tile;
			return;
		}
		for (int i = 0; i < (int)sections.size(); i++) {
			if (sections.at(i).x == (int)floor((float)x / static_cast<float>(SECT_SIZE)) && sections.at(i).y == (int)floor((float)y / static_cast<float>(SECT_SIZE))) {
				sections.at(i).bgTiles[mod(x, SECT_SIZE)][mod(y, SECT_SIZE)] = tile;
				return;
			}
		}
	}
	Tile getLocalGSBGTile(int x, int y) {
		if (x >= 0 && y >= 0 && x < SECT_SIZE && y < SECT_SIZE) {
			return gs.bgTiles[x][y];
		}
		for (int i = 0; i < (int)sections.size(); i++) {
			if (sections.at(i).x == (int)floor((float)x / static_cast<float>(SECT_SIZE)) && sections.at(i).y == (int)floor((float)y / static_cast<float>(SECT_SIZE))) {
				return sections.at(i).bgTiles[mod(x, SECT_SIZE)][mod(y, SECT_SIZE)];
			}
		}
		return {ttypes::air}; // do do replace
	}
	void generateTileSection(int x, int y) {
		gs = {x, y};
		// 1: base terrain gen
		for (int rx = 0; rx < SECT_SIZE; rx++) {
			float height = getGeneratorHeight(rx + x * SECT_SIZE);
			for (int ry = 0; ry < SECT_SIZE; ry++) {
				Tile t = generateTile(rx + x * SECT_SIZE, ry + y * SECT_SIZE, height, true);
				setLocalGSTile(rx, ry, t);
				setLocalGSBGTile(rx, ry, t);
				gs.lightmap[rx][ry] = gs.currentLightmap[rx][ry] = {0.f, 0.f, 0.f};
				gs.lightmapN[rx][ry] = 0;
			}
		}
		// 2: tree n sentry shack generation
		for (int rx = 0; rx < SECT_SIZE; rx++) {
			for (int ry = 0; ry < SECT_SIZE; ry++) {
				if (getLocalGSTile(rx, ry).type == ttypes::air && getLocalGSTile(rx, ry - 1).type != ttypes::air && randFloat() < 0.01f) {
					setLocalGSTile(rx, ry, {ttypes::sentry_shack_bottom});
					setLocalGSTile(rx, ry + 1, {ttypes::sentry_shack_middle});
					setLocalGSTile(rx, ry + 2, {ttypes::sentry_shack_top});
				}
				if (getLocalGSTile(rx, ry).type == ttypes::air && getLocalGSTile(rx, ry - 1).type == ttypes::dirt && randFloat() < 0.05f) {
					setLocalGSTile(rx, ry, {ttypes::log});
					setLocalGSTile(rx, ry + 1, {ttypes::log});
					setLocalGSTile(rx, ry + 2, {ttypes::log});
					setLocalGSTile(rx, ry + 3, {ttypes::log});
					setLocalGSTile(rx, ry + 4, {ttypes::log});
					setLocalGSTile(rx, ry + 5, {ttypes::log});
					setLocalGSTile(rx, ry + 6, {ttypes::leaves});
					setLocalGSTile(rx, ry + 7, {ttypes::leaves});
					setLocalGSTile(rx, ry + 8, {ttypes::leaves});
					setLocalGSTile(rx - 1, ry + 6, {ttypes::leaves});
					setLocalGSTile(rx - 1, ry + 7, {ttypes::leaves});
					setLocalGSTile(rx - 1, ry + 8, {ttypes::leaves});
					setLocalGSTile(rx + 1, ry + 6, {ttypes::leaves});
					setLocalGSTile(rx + 1, ry + 7, {ttypes::leaves});
					setLocalGSTile(rx + 1, ry + 8, {ttypes::leaves});
					
					setLocalGSTile(rx - 2, ry + 6, {ttypes::leaves});
					setLocalGSTile(rx - 2, ry + 7, {ttypes::leaves});
					setLocalGSTile(rx + 2, ry + 6, {ttypes::leaves});
					setLocalGSTile(rx + 2, ry + 7, {ttypes::leaves});
				}
			}
		}
		cout << "new size: " << sizeof(gs) << endl;
		return sections.push_back(gs);
	}
	TileAddress getTileAddress(int x, int y) {
		for (int i = 0; i < (int)sections.size(); i++) {
			if (sections.at(i).x == (int)floor((float)x / static_cast<float>(SECT_SIZE)) && sections.at(i).y == (int)floor((float)y / static_cast<float>(SECT_SIZE))) return {i, mod(x, SECT_SIZE), mod(y, SECT_SIZE)};
		}
		return {-1, 0, 0};
	}
	void lightingStep(vector<Light> lights, float dt) {
		float photonSpeed = 0.5f;
		float howmanyper = 0.08f;

		// Emit photons from lights
		for (int i = 0; i < (int)lights.size(); i++) {
			for (float r = 0; r < PI * 2.f; r += howmanyper) {
				photons.push_back({
					lights.at(i).pos.x,
					lights.at(i).pos.y,
					sin(r) * photonSpeed,
					cos(r) * photonSpeed,
					lights.at(i).intensity.r,
					lights.at(i).intensity.g,
					lights.at(i).intensity.b
				});
			}
		}

		// emit skylight from top 
		int right = floor(camera.right(0.f));
		for (int x = floor(camera.left(0.f)); x < right; x++) {
			photons.push_back({
				(float)x + 0.5f,
				camera.top(0.f),
				0.f,
				-1.f,
				0.5f,
				0.5f,
				0.5f
			});
		}

		// clear current light map
		for (int i = 0; i < (int)sections.size(); i++) {
			sections.at(i).clearCurrentLightmap();
		}

		// coop over every photon
		for (int i = (int)photons.size() - 1; i >= 0; i--) {
			for (int j = 0; j < 15; j++) { // rt steps per photon
				// Delete photon if it is bad
				if (
					(photons.at(i).r + photons.at(i).g + photons.at(i).b) / 3.f <= 0.f ||
					(
						(
							photons.at(i).x > camera.right(0.f) ||
							photons.at(i).x < camera.left(0.f) ||
							photons.at(i).y < camera.bottom(0.f) ||
							photons.at(i).y > camera.top(0.f)
						) &&
						!lineRect(
							photons.at(i).x,
							photons.at(i).y,
							photons.at(i).x + photons.at(i).xv * 32767.f,
							photons.at(i).y + photons.at(i).yv * 32767.f,
							camera.left(0.f),
							camera.bottom(0.f),
							camera.right(0.f) - camera.left(0.f),
							camera.top(0.f) - camera.bottom(0.f)
						)
					)
				) {
					photons.erase(photons.begin() + i);
					break;
				}
				
				// X movement
				if (isPointAir(photons.at(i).x + photons.at(i).xv, photons.at(i).y)) photons.at(i).x += photons.at(i).xv;
				else { // diffuse
					addColorToRGAndBInCurrentLightmapTile((int)floor(photons.at(i).x + photons.at(i).xv), (int)floor(photons.at(i).y), photons.at(i).r, photons.at(i).g, photons.at(i).b);
					photons.at(i).xv *= -1.f;
					rotateVector(&photons.at(i).xv, &photons.at(i).yv, randFloat() * 3.14f - 1.57f);
					photons.at(i).r -= 0.1f;
					photons.at(i).g -= 0.1f;
					photons.at(i).b -= 0.1f;
				}
				
				// Y movement
				if (isPointAir(photons[i].x, photons[i].y + photons[i].yv)) photons[i].y += photons[i].yv;
				else {
					addColorToRGAndBInCurrentLightmapTile((int)floor(photons.at(i).x), (int)floor(photons.at(i).y + photons.at(i).yv), photons.at(i).r, photons.at(i).g, photons.at(i).b);
					photons.at(i).yv *= -1.f;
					rotateVector(&photons.at(i).xv, &photons.at(i).yv, randFloat() * 3.14f - 1.57f);
					photons.at(i).r -= 0.1f;
					photons.at(i).g -= 0.1f;
					photons.at(i).b -= 0.1f;
				}
				
				addColorToRGAndBInCurrentLightmapTile((int)floor(photons.at(i).x), (int)floor(photons.at(i).y), photons.at(i).r, photons.at(i).g, photons.at(i).b);
			}
		}

		// Solve light map
		for (int i = 0; i < (int)sections.size(); i++) {
			sections.at(i).solveLightmap();
		}
	}
	Tile getTile(int x, int y) {
		for (int i = 0; i < (int)sections.size(); i++) {
			if (sections.at(i).x == (int)floor((float)x / static_cast<float>(SECT_SIZE)) && sections.at(i).y == (int)floor((float)y / static_cast<float>(SECT_SIZE))) return sections.at(i).tiles[mod(x, SECT_SIZE)][mod(y, SECT_SIZE)];
		}
		return {ttypes::air};
	}
	void addColorToRGAndBInCurrentLightmapTile(int x, int y, float r, float g, float b) {
		for (int i = 0; i < (int)sections.size(); i++) {
			if (sections.at(i).x == (int)floor((float)x / static_cast<float>(SECT_SIZE)) && sections.at(i).y == (int)floor((float)y / static_cast<float>(SECT_SIZE))) {
				sections.at(i).currentLightmap[mod(x, SECT_SIZE)][mod(y, SECT_SIZE)].r += r;
				sections.at(i).currentLightmap[mod(x, SECT_SIZE)][mod(y, SECT_SIZE)].g += g;
				sections.at(i).currentLightmap[mod(x, SECT_SIZE)][mod(y, SECT_SIZE)].b += b;
			}
		}
	}
	void setToRGAndBInCurrentLightmapTile(int x, int y, float r, float g, float b) {
		for (int i = 0; i < (int)sections.size(); i++) {
			if (sections.at(i).x == (int)floor((float)x / static_cast<float>(SECT_SIZE)) && sections.at(i).y == (int)floor((float)y / static_cast<float>(SECT_SIZE))) {
				sections.at(i).currentLightmap[mod(x, SECT_SIZE)][mod(y, SECT_SIZE)].r += r;
				sections.at(i).currentLightmap[mod(x, SECT_SIZE)][mod(y, SECT_SIZE)].g += g;
				sections.at(i).currentLightmap[mod(x, SECT_SIZE)][mod(y, SECT_SIZE)].b += b;
			}
		}
	}
	Tile getBgTile(int x, int y) {
		for (int i = 0; i < (int)sections.size(); i++) {
			if (sections.at(i).x == (int)floor((float)x / static_cast<float>(SECT_SIZE)) && sections.at(i).y == (int)floor((float)y / static_cast<float>(SECT_SIZE))) return sections.at(i).bgTiles[mod(x, SECT_SIZE)][mod(y, SECT_SIZE)];
		}
		return {ttypes::air};
	}
	bool isPointAir(float x, float y) {
		return getTile((int)x, (int)y).type == ttypes::air;
	}
	void setTile(int x, int y, Tile t) {
		for (int i = 0; i < (int)sections.size(); i++) {
			if (sections.at(i).x == (int)floor((float)x / static_cast<float>(SECT_SIZE)) && sections.at(i).y == (int)floor((float)y / static_cast<float>(SECT_SIZE))) sections.at(i).tiles[mod(x, SECT_SIZE)][mod(y, SECT_SIZE)] = t;
		}
		// do do tile queueueueue
	}
	void damageTile(int x, int y, float amount) {
		for (int i = 0; i < (int)sections.size(); i++) {
			if (sections.at(i).x == (int)floor((float)x / static_cast<float>(SECT_SIZE)) && sections.at(i).y == (int)floor((float)y / static_cast<float>(SECT_SIZE))) {
				sections.at(i).tiles[mod(x, SECT_SIZE)][mod(y, SECT_SIZE)].health -= amount;
				if (sections.at(i).tiles[mod(x, SECT_SIZE)][mod(y, SECT_SIZE)].health <= 0.f) {
					Entity* e = new EntityItem();
					e->items.at(0) = sections.at(i).tiles[mod(x, SECT_SIZE)][mod(y, SECT_SIZE)].drops;
					e->pos.x = (float)floor(x) + 0.5f;
					e->pos.y = (float)floor(y) + 0.5f;
					spawnEntity(e);
					sections.at(i).tiles[mod(x, SECT_SIZE)][mod(y, SECT_SIZE)] = {ttypes::air};
				}
				break;
			}
		}
	}
	bool areTwoEntitiesCollidingWithEachother(Entity* e1, Entity* e2) {
		if (e1->id == e2->id) return false;
		return e1->pos.x + e1->size.x / 2.f > e2->pos.x - e2->size.x / 2.f && e1->pos.x - e1->size.x / 2.f < e2->pos.x + e2->size.x / 2.f && e1->pos.y + e1->size.y > e2->pos.y && e1->pos.y < e2->pos.y + e2->size.y;
	}
	EntityCollision getEntityCollision(Entity* e) {
		vector<iVec2> tileCollisions;
		bool collided = false;
		for (int i = 0; i < (int)sections.size(); i++) {
			if (
				e->pos.x + e->size.x / 2.f < (float)sections.at(i).x * static_cast<float>(SECT_SIZE) ||
				e->pos.x - e->size.x / 2.f > ((float)sections.at(i).x + 1.f) * static_cast<float>(SECT_SIZE) ||
				e->pos.y + e->size.y < (float)sections.at(i).y * static_cast<float>(SECT_SIZE) ||
				e->pos.y > ((float)sections.at(i).y + 1.f) * static_cast<float>(SECT_SIZE)
			) continue;
			for (int x = max((int)floor(e->pos.x - e->size.x / 2.f), sections.at(i).x * SECT_SIZE); x < min((int)ceil(e->pos.x + e->size.x / 2.f), (sections.at(i).x + 1) * SECT_SIZE); x++) {
				for (int y = max((int)floor(e->pos.y), sections.at(i).y * SECT_SIZE); y < min((int)ceil(e->pos.y + e->size.y), (sections.at(i).y + 1) * SECT_SIZE); y++) {
					if (!sections.at(i).tiles[mod(x, SECT_SIZE)][mod(y, SECT_SIZE)].isSolid) continue;
					tileCollisions.push_back({x, y});
					collided = true;
				}
			}
		}
		if (collided) return {true, tileCollisions, false, e, nullptr};
		/*for (int i = 0; i < (int)entities.size(); i++) {
			if (entities.at(i)->id == e->id) continue;
			if (areTwoEntitiesCollidingWithEachother(e, entities.at(i))) return {true, {}, true, e, entities.at(i)};
		}*/
		return {false, {}, false, e, nullptr};
	}
	void makeEntityPunch(Entity* e) {
		if (e->punchDelayTimer > 0.f) return;
		// cout << "punching intity info--- id: " << e->id << ", type" << e->type << endl;
		for (int i = 0; i < (int)entities.size(); i++) {
			if (entities.at(i)->id == e->id) {
				break;
			}
		}
		e->punchDelayTimer = e->items.at(e->itemNumber).punchDelay;
		e->swingRotation = 0.f;
		e->isSwinging = true;
		bool facingRight = e->facingVector.x != -1;
		particles.push_back({{e->pos.x + (facingRight ? e->size.x / 2.f + 0.5f : e->size.x / -2.f - 0.5f), e->pos.y + e->size.y / 2.f}, {0.f, 0.f}, {facingRight ? 1.f : -1.f, 1.f}, 0.3f, "sweep"});
		for (int i = (int)e->size.y; i >= 0; i--) {
			if (getTile((int)floor(e->pos.x) + (facingRight ? 1 : -1), (int)floor(e->pos.y) + i).type != ttypes::air) {
				damageTile((int)floor(e->pos.x) + (facingRight ? 1 : -1), (int)floor(e->pos.y) + i, e->items.at(e->itemNumber).damage);
				break;
			}
		}
		float right = facingRight ? (e->pos.x + e->size.x / 2.f + 1.f) : (e->pos.x - e->size.x / 2.f);
		float left = facingRight ? (e->pos.x + e->size.x / 2.f) : (e->pos.x - e->size.x / 2.f - 1.f);
		for (int i = 0; i < (int)entities.size(); i++) {
			if (!entities.at(i)->isDamagable) continue;
			if (
				right > entities.at(i)->pos.x - entities.at(i)->size.x / 2.f && 
				left < entities.at(i)->pos.x + entities.at(i)->size.x / 2.f && 
				e->pos.y + e->size.y / 2.f > entities.at(i)->pos.y - entities.at(i)->size.y / 2.f && 
				e->pos.y - e->size.y / 2.f < entities.at(i)->pos.y + entities.at(i)->size.y / 2.f
			) {
				particles.push_back({{entities.at(i)->pos.x, entities.at(i)->pos.y + entities.at(i)->size.y}, {0.f, 1.f}, {1.f, 1.f}, 1.f, "damage_heart"});
				entities.at(i)->health -= e->items.at(e->itemNumber).damage;
				entities.at(i)->damageRedness = 1.f;
			}
		}
	}
	void makeEntityDrop(Entity* e) {
		bool facingRight = e->facingVector.x != -1;
		Entity* item = new EntityItem();
		item->items.at(0) = e->items.at(e->itemNumber);
		item->pos.x = e->pos.x;
		item->pos.y = e->pos.y + e->size.y - item->size.y;
		item->vel.x = facingRight ? 1.f : -1.f;
		e->items.at(e->itemNumber) = itypes::none;
		spawnEntity(item);
	}
	void spawnEntity(Entity* e) {
		entities.push_back(e);
		if (self == -1 && e->type == etypes::player) self = entities.size() - 1;
	}
	void deleteEntity(int index) {
		delete entities.at(index);
		entities.erase(entities.begin() + index);
		if (index < self) self--;
		else if (index == self) {
			self = -1;
			spawnEntity(new EntityPlayer());
		}
	}
};
float lerpd(float a, float b, float t, float d) {
	return lerp(a, b, 1.f - powf((1.f - t), d));
}
class GameState {
	public:
	World world;
	float dt = 1.f;
	float x = 0.f;
	bool playing = true;
	bool shopOpen = false;
	bool commandPromptOpen = false;
	int typingMode = 0; // 0 in game 1 typing
	string commandPromptText = "";
	void tick() {
		controls.previousClipMouse = controls.clipMouse;
		controls.clipMouse.x = (controls.mouse.x / (float)width - 0.5f) * 2.f;
		controls.clipMouse.y = (0.5f - controls.mouse.y / (float)height) * 2.f;

		if (shopOpen) return;

		controls.worldMouse.x = controls.clipMouse.x * (world.camera.zoom * (float)width / (float)height) + world.camera.pos.x;
		controls.worldMouse.y = controls.clipMouse.y * world.camera.zoom + world.camera.pos.y;
		// Physics Tracing Extreme
		float gravity = -9.807f;

		if (controls.space) {
			Entity* e = new EntitySentry();
			e->pos.x = randFloat() * 100.f;
			if (!world.getEntityCollision(e).collided) world.entities.push_back(e);
		}

		//find player is
		vector<int> playerIs;
		vector<Light> lights;
		for (int i = 0; i < (int)world.entities.size(); i++) {
			Entity* e = world.entities.at(i);
			if (e->type == etypes::player) {
				e->health -= dt * 0.1f;
				if (controls.e && e->items.at(e->itemNumber).isEdible) {
					e->health += e->items.at(e->itemNumber).eatingHealth * dt;
					if (e->health > e->maxHealth) {
						e->health = e->maxHealth;
					}
					e->items.at(e->itemNumber).durability -= dt;
					if (e->items.at(e->itemNumber).durability <= 0.f) {
						e->items.at(e->itemNumber) = {itypes::none};
					}
				}
				playerIs.push_back(i);
				if (controls.xPressed) {
					controls.xPressed = false;
					world.makeEntityPunch(world.entities.at(i));
				}
				if (controls.bPressed) {
					controls.bPressed = false;
					world.makeEntityDrop(world.entities.at(i));
				}
				if (
					e->items.at(e->itemNumber).emission.r > 0.f &&
					e->items.at(e->itemNumber).emission.g > 0.f &&
					e->items.at(e->itemNumber).emission.b > 0.f
				) lights.push_back({{e->pos.x, e->pos.y + e->size.y / 2.f}, e->items.at(e->itemNumber).emission});
				e->items.at(e->itemNumber).emission.r *= 0.9999f;
				e->items.at(e->itemNumber).emission.g *= 0.9999f;
				e->items.at(e->itemNumber).emission.b *= 0.9999f;
			}
		}
		world.lightingStep(lights, dt);
		if (controls.mouseDown) {
			world.damageTile((int)floor(controls.worldMouse.x), (int)floor(controls.worldMouse.y), 53.f * dt);
		}
		for (int i = (int)world.entities.size() - 1; i >= 0; i--) {
			Entity* e = world.entities.at(i);
			//newest new est newsest newest newest newest newest latest
			e->punchDelayTimer -= dt;
			if (e->punchDelayTimer < 0.f) e->punchDelayTimer = 0.f;

			if (e->type == etypes::sentry) {
				e->particleEmitTimer -= dt;
				while (e->particleEmitTimer <= 0.f) {
					e->particleEmitTimer += e->particleEmitDelay;
					float theta = randFloat() * PI * 2.f;
					world.particles.push_back({{e->pos.x, e->pos.y + e->size.y / 2.f}, {sin(theta), cos(theta)}, {0.5f, 0.5f}, 2.f, "dark_energy"});
				}
			}

			bool controlsLeft = false;
			bool controlsRight = false;
			bool controlsUp = false;
			if (e->controlsType == 0) {
				controlsLeft = controls.a || controls.left;
				controlsRight = controls.d || controls.right;
				controlsUp = controls.w || controls.up;
			} else if (e->controlsType == 1) {
				int nearestPlayerI = -1;
				float nearestPlayerDistance = 0.f;
				for (int j = 0; j < (int)playerIs.size(); j++) {
					float currentDistance = distance(world.entities.at(playerIs.at(j))->pos, e->pos);
					if ((nearestPlayerI == -1 || currentDistance < nearestPlayerDistance) && world.entities.at(playerIs.at(j))->id != e->id) {
						nearestPlayerDistance = currentDistance;
						nearestPlayerI = j;
					}
				}
				if (nearestPlayerI != -1) {
					controlsUp = true;
					controlsLeft = (world.entities.at(playerIs.at(nearestPlayerI))->pos.x < e->pos.x);
					controlsRight = (world.entities.at(playerIs.at(nearestPlayerI))->pos.x > e->pos.x);
					world.makeEntityPunch(e);
				}
			} else if (e->controlsType == 2) {
				int nearestPlayerI = -1;
				float nearestPlayerDistance = 0.f;
				for (int j = 0; j < (int)playerIs.size(); j++) {
					float currentDistance = distance(world.entities.at(playerIs.at(j))->pos, e->pos);
					if ((nearestPlayerI == -1 || currentDistance < nearestPlayerDistance) && world.entities.at(playerIs.at(j))->id != e->id) {
						nearestPlayerDistance = currentDistance;
						nearestPlayerI = j;
					}
				}
				if (nearestPlayerI != -1) {
					controlsLeft = (controls.a || controls.left) && (world.entities.at(playerIs.at(nearestPlayerI))->pos.x < e->pos.x);
					controlsRight = (controls.d || controls.right) && (world.entities.at(playerIs.at(nearestPlayerI))->pos.x > e->pos.x);
					controlsUp = controls.w || controls.up;
					world.makeEntityPunch(e);
				}
			}
			if (e->isSwinging) {
				e->swingRotation += 15.f * dt;
				if (e->swingRotation > PI) {
					e->swingRotation = 0.f;
					e->isSwinging = false;
				}
			}

			float friction;
			if (e->onGround) {
				Tile frictionTile = world.getTile((int)floor(e->pos.x), (int)floor(e->pos.y - 0.1f));
				if (frictionTile.type == ttypes::air) {
					float one = 1.f;
					if (modf(e->pos.x, &one) > 0.5f) {
						frictionTile = world.getTile((int)e->pos.x + 1, (int)(e->pos.y - 0.1f));
					} else {
						frictionTile = world.getTile((int)e->pos.x - 1, (int)(e->pos.y - 0.1f));
					}
				}
				friction = frictionTile.friction;
			} else {
				friction = 0.5f;
			}
			
			e->vel.y += gravity * dt;
			float netMovement = controlsRight - controlsLeft;
			if (netMovement != 0.f) {
				e->facingVector = {(int)netMovement, 0};
			}
			e->vel.x = lerpd(e->vel.x, netMovement * (controls.shift ? 6.f : e->speed), friction, dt);

			//x collision
			e->pos.x += e->vel.x * dt;
			EntityCollision collision = world.getEntityCollision(e);
			if (collision.collided) {
				e->pos.x -= e->vel.x * dt;
				e->vel.x = 0.f;
			}
			
			//y collision n stuff
			e->pos.y += e->vel.y * dt;
			float oldOnGround = e->onGround;
			e->onGround = false;
			collision = world.getEntityCollision(e);
			if (collision.collided) {
				e->pos.y -= e->vel.y * dt;
				e->onGround = e->vel.y < 0.f;
				if (!oldOnGround && e->onGround && e->isDamagable && e->vel.y <= -10.f) {
					e->health += (e->vel.y + 10.f) / 5.f;
					e->damageRedness = 1.f;
				}
				e->vel.y = 0.f;
				if (controlsUp && e->onGround) {
					e->vel.y = 6.f;
				}
			}
			if (controlsUp && debug) {
				e->vel.y = 6.f;
			}
			e->damageRedness = max(e->damageRedness - dt / 0.5f, 0.f);
			if (e->canPickUpStuff) {
				for (int i = (int)world.entities.size() - 1; i >= 0; i--) {
					if (world.entities.at(i)->type != etypes::item || world.entities.at(i)->age < 1.f || world.entities.at(i)->id == e->id) continue;
					if (world.areTwoEntitiesCollidingWithEachother(e, world.entities.at(i))) {
						for (int j = 0; j < (int)e->items.size(); j++) {
							if (e->items.at(j).type == itypes::none) {
								e->items.at(j) = world.entities.at(i)->items.at(0);
								world.deleteEntity(i);
								break;
							}
						}
					}
				}
			}
			if (e->type == etypes::player) {
				for (int x = (int)floor(e->pos.x - e->size.x / 2.f); x < (int)ceil(e->pos.x + e->size.x / 2.f); x++) {
					for (int y = (int)floor(e->pos.y); y < (int)ceil(e->pos.y + e->size.y); y++) {
						if (world.getTile(x, y).type == ttypes::sentry_shack_middle) {
							world.setTile(x, y - 1, {ttypes::air});
							world.setTile(x, y, {ttypes::air});
							world.setTile(x, y + 1, {ttypes::air});
							Entity* e = new EntitySentry();
							e->pos.x = (float)x;
							e->pos.y = (float)y;
							world.spawnEntity(e);
						}
					}
				}
			}
			e->age += dt;
		}

		if ((int)playerIs.size() > 0) {
			float targetX = 0.f;
			float targetY = 0.f;
			for (int i = 0; i < (int)playerIs.size(); i++) {
				targetX += world.entities.at(playerIs.at(i))->pos.x;
				targetY += world.entities.at(playerIs.at(i))->pos.y + world.entities.at(playerIs.at(i))->size.y / 2.f;
			}
			targetX /= (float)playerIs.size();
			targetY /= (float)playerIs.size();

			world.camera.pos.x = lerpd(world.camera.pos.x, targetX, 0.99f, dt);
			world.camera.pos.y = lerpd(world.camera.pos.y, targetY, 0.99f, dt);
		}

		for (int i = (int)world.entities.size() - 1; i >= 0; i--) {
			if (world.entities.at(i)->health <= 0.f) {
				world.particles.push_back({world.entities.at(i)->pos, {0.f, 1.f}, {1.f, 1.f}, 2.f, "skull"});
				world.deleteEntity(i);
			}
		}
		for (int i = (int)world.particles.size() - 1; i >= 0; i--) {
			world.particles.at(i).pos.x += world.particles.at(i).vel.x * dt;
			world.particles.at(i).pos.y += world.particles.at(i).vel.y * dt;
			world.particles.at(i).time -= dt;
			if (world.particles.at(i).time <= 0) world.particles.erase(world.particles.begin() + i);
		}
		world.camera.zoom = lerpd(world.camera.zoom, world.camera.zoomTarget, 0.99f, dt);
		// delete old and generate new sections
		for (int i = world.sections.size() - 1; i >= 0; i--) {
			if (
				(float)((world.sections.at(i).x + 1) * SECT_SIZE) < world.camera.left(0.f) ||
				(float)(world.sections.at(i).x * SECT_SIZE) > world.camera.right(0.f) ||
				(float)((world.sections.at(i).y + 1) * SECT_SIZE) < world.camera.bottom(0.f) ||
				(float)(world.sections.at(i).y * SECT_SIZE) > world.camera.top(0.f)
			) {
				world.sections.erase(world.sections.begin() + i);
			}
		};
		for (int x = floor(world.camera.left(0.f) / static_cast<float>(SECT_SIZE)); x <= floor(world.camera.right(0.f) / static_cast<float>(SECT_SIZE)); x++) {
			for (int y = floor(world.camera.bottom(0.f) / static_cast<float>(SECT_SIZE)); y <= floor(world.camera.top(0.f) / static_cast<float>(SECT_SIZE)); y++) {
				bool has = false;
				for (int i = 0; i < (int)world.sections.size(); i++) {
					if (world.sections[i].x == x && world.sections[i].y == y) {
						has = true;
						break;
					}
				}
				if (has) continue;
				world.generateTileSection(x, y);
			}
		}

		world.time += dt;
	}
	int executeCommand(string text) {
		vector<string> args = {};// loop over every and if
		string currentArg = "";
		for (int i = 0; i <= (int)text.length(); i++) {
			if (i == (int)text.length() || text.at(i) == ' ') {
				args.push_back(currentArg);
				currentArg = "";
			} else {
				currentArg += text.at(i);
			}
		}
		int argC = args.size();
		if (args.at(0) == "tp" && (argC == 2 || argC == 3)) {
			world.entities.at(world.self)->pos.x = stof(args.at(1));
			if (argC == 3) world.entities.at(world.self)->pos.y = stof(args.at(2));
		} else if (args.at(0) == "spawn" && (argC == 2 || argC == 4)) {
			Entity* e = nullptr;
			if (args.at(1) == "sentry") {
				e = new EntitySentry();
			} else if (args.at(1) == "mimic") {
				e = new EntityMimic();
			} else if (args.at(1) == "merchant") {
				e = new EntityMerchant();
			} else {
				e = new EntitySentry();
			}
			e->pos = world.entities.at(world.self)->pos;
			if (argC == 4) {
				e->pos.x += stof(args.at(2));
				e->pos.y += stof(args.at(3));
			}
			world.spawnEntity(e);
		} else if (args.at(0) == "generate" && argC == 3) {
			world.generateTileSection(stoi(args.at(1)), stoi(args.at(2)));
		}
		return 0;
	}
};
GameState game;
struct Button {
	string text;
	int action;
	float x;
	float y;
	float width;
	float height;
	bool enabled;
	bool visible;
};
vector<Button> buttons = {
	{"SHOP", 0, 0.75f, 0.75f, 0.2f, 0.2f, true, true},
	{"next skin", 1, -0.5f, 0.1f, 0.3f, 0.1f, true, false},
	{"vsync: on", 2, -0.5f, 0.f, 0.3f, 0.1f, true, false},
};
void shopButton() {
	game.shopOpen = !game.shopOpen;
	buttons.at(1).visible = game.shopOpen;
	buttons.at(2).visible = game.shopOpen;
}
void skinButton() {
	int skinNumber = 0;
	for (int i = 0; i < (int)skins.list.size(); i++) {
		if (game.world.entities.at(game.world.self)->material == "skin_" + skins.list.at(i)) {
			skinNumber = i + 1;
			if (skinNumber >= skins.list.size()) skinNumber = 0;
			game.world.entities.at(game.world.self)->material = "skin_" + skins.list.at(skinNumber);
			cout << "[INFO] changed skin to: " + skins.list.at(skinNumber) << endl;
			break;
		}
	}
}
bool vsync = true;
void toggleVsync() {
	if (vsync = !vsync) {
		glfwSwapInterval(1);
	} else {
		glfwSwapInterval(0);
	}
}
void vsyncButton() {
	toggleVsync();
	buttons.at(2).text = string("vsync: ") + (vsync ? "on" : "off");
}
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (game.typingMode == 0) {
		if (action == GLFW_PRESS) {
			if (key == GLFW_KEY_ESCAPE) {
				shopButton();
			} else
			if (key == GLFW_KEY_Q) {
				glfwSetWindowShouldClose(window, GLFW_TRUE);
			}
			else if (key == GLFW_KEY_W) controls.w = true;
			else if (key == GLFW_KEY_A) controls.a = true;
			else if (key == GLFW_KEY_S) controls.s = true;
			else if (key == GLFW_KEY_D) controls.d = true;
			else if (key == GLFW_KEY_SLASH) {
				game.commandPromptOpen = true;
				game.typingMode = 1;
			}
			else if (key == GLFW_KEY_1) game.world.entities.at(game.world.self)->itemNumber = 0;
			else if (key == GLFW_KEY_2) game.world.entities.at(game.world.self)->itemNumber = 1;
			else if (key == GLFW_KEY_3) game.world.entities.at(game.world.self)->itemNumber = 2;
			else if (key == GLFW_KEY_4) game.world.entities.at(game.world.self)->itemNumber = 3;
			else if (key == GLFW_KEY_5) game.world.entities.at(game.world.self)->itemNumber = 4;
			else if (key == GLFW_KEY_6) game.world.entities.at(game.world.self)->itemNumber = 5;
			else if (key == GLFW_KEY_7) game.world.entities.at(game.world.self)->itemNumber = 6;
			else if (key == GLFW_KEY_8) game.world.entities.at(game.world.self)->itemNumber = 7;
			else if (key == GLFW_KEY_LEFT_SHIFT) controls.shift = true;
			else if (key == GLFW_KEY_UP) controls.up = true;
			else if (key == GLFW_KEY_DOWN) controls.down = true;
			else if (key == GLFW_KEY_LEFT) controls.left = true;
			else if (key == GLFW_KEY_RIGHT) controls.right = true;
			else if (key == GLFW_KEY_E) controls.e = true;
			else if (key == GLFW_KEY_V) debug = !debug;
			else if (key == GLFW_KEY_SPACE) {
				controls.space = true;
			}
			else if (key == GLFW_KEY_X) controls.xPressed = true;
			else if (key == GLFW_KEY_B) controls.bPressed = true;
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
			else if (key == GLFW_KEY_E) controls.e = false;
			else if (key == GLFW_KEY_SPACE) controls.space = false;
		}
	} else if (game.typingMode == 1) {
		if (action == GLFW_PRESS) {
			if (key == GLFW_KEY_A) game.commandPromptText += 'a';
			else if (key == GLFW_KEY_B) game.commandPromptText += 'b';
			else if (key == GLFW_KEY_C) game.commandPromptText += 'c';
			else if (key == GLFW_KEY_D) game.commandPromptText += 'd';
			else if (key == GLFW_KEY_E) game.commandPromptText += 'e';
			else if (key == GLFW_KEY_F) game.commandPromptText += 'f';
			else if (key == GLFW_KEY_G) game.commandPromptText += 'g';
			else if (key == GLFW_KEY_H) game.commandPromptText += 'h';
			else if (key == GLFW_KEY_I) game.commandPromptText += 'i';
			else if (key == GLFW_KEY_J) game.commandPromptText += 'j';
			else if (key == GLFW_KEY_K) game.commandPromptText += 'k';
			else if (key == GLFW_KEY_L) game.commandPromptText += 'l';
			else if (key == GLFW_KEY_M) game.commandPromptText += 'm';
			else if (key == GLFW_KEY_N) game.commandPromptText += 'n';
			else if (key == GLFW_KEY_O) game.commandPromptText += 'o';
			else if (key == GLFW_KEY_P) game.commandPromptText += 'p';
			else if (key == GLFW_KEY_Q) game.commandPromptText += 'q';
			else if (key == GLFW_KEY_R) game.commandPromptText += 'r';
			else if (key == GLFW_KEY_S) game.commandPromptText += 's';
			else if (key == GLFW_KEY_T) game.commandPromptText += 't';
			else if (key == GLFW_KEY_U) game.commandPromptText += 'u';
			else if (key == GLFW_KEY_V) game.commandPromptText += 'v';
			else if (key == GLFW_KEY_W) game.commandPromptText += 'w';
			else if (key == GLFW_KEY_X) game.commandPromptText += 'x';
			else if (key == GLFW_KEY_Y) game.commandPromptText += 'y';
			else if (key == GLFW_KEY_Z) game.commandPromptText += 'z';
			else if (key == GLFW_KEY_0) game.commandPromptText += '0';
			else if (key == GLFW_KEY_1) game.commandPromptText += '1';
			else if (key == GLFW_KEY_2) game.commandPromptText += '2';
			else if (key == GLFW_KEY_3) game.commandPromptText += '3';
			else if (key == GLFW_KEY_4) game.commandPromptText += '4';
			else if (key == GLFW_KEY_5) game.commandPromptText += '5';
			else if (key == GLFW_KEY_6) game.commandPromptText += '6';
			else if (key == GLFW_KEY_7) game.commandPromptText += '7';
			else if (key == GLFW_KEY_8) game.commandPromptText += '8';
			else if (key == GLFW_KEY_9) game.commandPromptText += '9';
			else if (key == GLFW_KEY_SPACE) game.commandPromptText += ' ';
			else if (key == GLFW_KEY_BACKSPACE && (int)game.commandPromptText.size() > 0) game.commandPromptText.pop_back();
			else if (key == GLFW_KEY_ENTER) {
				game.executeCommand(game.commandPromptText);
				game.commandPromptOpen = false;
				game.typingMode = 0;
			}
		}
	}
}
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		double xpos, ypos;
		//getting cursor position
		glfwGetCursorPos(window, &xpos, &ypos);
		controls.mouseDown = true;
		for (int i = 0; i < (int)buttons.size(); i++) {
			if (buttons.at(i).visible && buttons.at(i).enabled && controls.clipMouse.x > buttons.at(i).x && controls.clipMouse.x < buttons.at(i).x + buttons.at(i).width && controls.clipMouse.y > buttons.at(i).y && controls.clipMouse.y < buttons.at(i).y + buttons.at(i).height) {
				if (buttons.at(i).action == 0) shopButton();
				if (buttons.at(i).action == 1) skinButton();
				if (buttons.at(i).action == 2) vsyncButton();
			}
		}
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		double xpos, ypos;
		//getting cursor position
		glfwGetCursorPos(window, &xpos, &ypos);
		controls.mouseDown = false;
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		double xpos, ypos;
		//getting cursor position
		glfwGetCursorPos(window, &xpos, &ypos);
		int type = game.world.entities.at(game.world.self)->items.at(game.world.entities.at(game.world.self)->itemNumber).type;
		bool didPlaceTile = false;
		int tileToPlace = ttypes::none;
		if (type == itypes::dirt) {
			didPlaceTile = true;
			tileToPlace = ttypes::dirt;
		} else if (type == itypes::snow) {
			didPlaceTile = true;
			tileToPlace = ttypes::snow;
		} else if (type == itypes::stone) {
			didPlaceTile = true;
			tileToPlace = ttypes::stone;
		} else if (type == itypes::wood) {
			didPlaceTile = true;
			tileToPlace = ttypes::wood;
		} else if (type == itypes::log) {
			didPlaceTile = true;
			tileToPlace = ttypes::log;
		} else if (type == itypes::leaves) {
			didPlaceTile = true;
			tileToPlace = ttypes::leaves;
		}
		if (didPlaceTile) {
			game.world.setTile((int)floor(controls.worldMouse.x), (int)floor(controls.worldMouse.y), {tileToPlace});
			game.world.entities.at(game.world.self)->items.at(game.world.entities.at(game.world.self)->itemNumber) = {itypes::none};
		}
	}
}
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	controls.mouse = {(float)xpos, (float)ypos};
}
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	game.world.camera.zoomTarget /= pow(1.1, yoffset);
}
static void iconify_callback(GLFWwindow* window, int iconified) {
	windowIconified = (iconified == GLFW_TRUE) ? true : false;
}
static void error_callback(int error, const char* description) {
	fprintf(stderr, "[ERROR] %s\n", description);
}

class GameStateRenderer {
public:
	GameState* game;

	float aspect;
	GLuint vertexBuffer,elementBuffer;

	struct Vertex {
		float x, y, z;
		float u, v;
		float tileHealth;
		float lr, lg, lb;
	};

	Shader solidV{"resources/shader/solid.vsh", GL_VERTEX_SHADER};
	Shader guiV{"resources/shader/gui.vsh", GL_VERTEX_SHADER};
	Shader fontV{"resources/shader/font.vsh", GL_VERTEX_SHADER};

	Shader solidF{"resources/shader/solid.fsh", GL_FRAGMENT_SHADER};
	Shader guiF{"resources/shader/gui.fsh", GL_FRAGMENT_SHADER};
	Shader guiGrayscaleF{"resources/shader/gui_grayscale.fsh", GL_FRAGMENT_SHADER};
	Shader healthBarGreenF{"resources/shader/health_bar_green.fsh", GL_FRAGMENT_SHADER};
	Shader healthBarBgF{"resources/shader/health_bar_background.fsh", GL_FRAGMENT_SHADER};
	Shader emptyF{"resources/shader/empty.fsh", GL_FRAGMENT_SHADER};

	vector<Material> materials;

	vector<Vertex> vertices = {};
	vector<vector<unsigned int>> materialIndices = {};

	int getMatID(string name) {
		int size = (int)materials.size();
		for (int i = 0; i < size; i++) {
			if (name == materials.at(i).name) return i;
		}
		return 0;
	}
	void addBothSpaceMaterial(string name, string texture) {
		materials.push_back({name, solidV.shader, solidF.shader, texture});
		materials.push_back({name + "_gui", guiV.shader, guiF.shader, texture});
	}
	GameStateRenderer(GameState* gam) {
		game = gam;
		//append materials

		//very background
		materials.push_back({"sky"						, solidV.shader	, guiF.shader			, "resources/texture/sky.png"});
		materials.push_back({"stone_background"			, solidV.shader	, solidF.shader			, "resources/texture/stone_background.png"});
		materials.push_back({"hills"					, solidV.shader	, solidF.shader			, "resources/texture/hills.png"});
		materials.push_back({"cloud"					, solidV.shader	, solidF.shader			, "resources/texture/cloud.png"});

		// tiles
		addBothSpaceMaterial("dirt"																, "resources/texture/dirt.png");
		addBothSpaceMaterial("snow"																, "resources/texture/snow.png");
		addBothSpaceMaterial("stone"															, "resources/texture/stone.png");
		addBothSpaceMaterial("wood"																, "resources/texture/wood.png");
		addBothSpaceMaterial("log"																, "resources/texture/log.png");
		addBothSpaceMaterial("leaves"															, "resources/texture/leaves.png");
		addBothSpaceMaterial("grass"															, "resources/texture/grass.png");
		addBothSpaceMaterial("sentry_shack_bottom"												, "resources/texture/sentry_shack_bottom.png");
		addBothSpaceMaterial("sentry_shack_middle"												, "resources/texture/sentry_shack_middle.png");
		addBothSpaceMaterial("sentry_shack_top"													, "resources/texture/sentry_shack_top.png");
		materials.push_back({"grass_left"				, solidV.shader	, solidF.shader			, "resources/texture/grass_left.png"});
		materials.push_back({"grass_right"				, solidV.shader	, solidF.shader			, "resources/texture/grass_right.png"});

		//entities and particles
		for (int i = 0; i < (int)skins.list.size(); i++) {
			addBothSpaceMaterial("skin_" + skins.list.at(i), "resources/texture/skins/" + skins.list.at(i) + ".png");
		}
		materials.push_back({"sentry"					, solidV.shader	, solidF.shader			, "resources/texture/sentry.png"});
		materials.push_back({"mimic"					, solidV.shader	, solidF.shader			, "resources/texture/mimic.png"});
		materials.push_back({"merchant"					, solidV.shader	, solidF.shader			, "resources/texture/merchant.png"});
		materials.push_back({"tile_cracks"				, solidV.shader	, solidF.shader			, "resources/texture/tile_cracks.png"});
		materials.push_back({"select"					, solidV.shader	, solidF.shader			, "resources/texture/select.png"});
		materials.push_back({"dark_energy"				, solidV.shader	, solidF.shader			, "resources/texture/dark_energy.png"});
		materials.push_back({"skull"					, solidV.shader	, solidF.shader			, "resources/texture/skull.png"});
		materials.push_back({"damage_heart"				, solidV.shader	, solidF.shader			, "resources/texture/damage_heart.png"});
		materials.push_back({"sweep"					, solidV.shader	, solidF.shader			, "resources/texture/sweep.png"});
		materials.push_back({"outline"					, solidV.shader	, solidF.shader			, "resources/texture/outline.png"});

		// items
		addBothSpaceMaterial("sword"															, "resources/texture/sword.png");
		addBothSpaceMaterial("excalibur"														, "resources/texture/excalibur.png");
		addBothSpaceMaterial("burger"															, "resources/texture/burger.png");
		addBothSpaceMaterial("antbread"															, "resources/texture/antbread.png");
		addBothSpaceMaterial("lantern"															, "resources/texture/lantern.png");
		addBothSpaceMaterial("pickaxe"															, "resources/texture/pickaxe.png");

		// gui
		materials.push_back({"blood_vignette"			, guiV.shader	, guiF.shader			, "resources/texture/blood_vignette.png"});
		materials.push_back({"items_slot"				, guiV.shader	, guiF.shader			, "resources/texture/items_slot.png"});
		materials.push_back({"items_selected"			, guiV.shader	, guiF.shader			, "resources/texture/items_selected.png"});
		materials.push_back({"health_green"				, guiV.shader	, healthBarGreenF.shader, "resources/texture/items_selected.png"});
		materials.push_back({"health_bg"				, guiV.shader	, healthBarBgF.shader	, "resources/texture/items_selected.png"});
		materials.push_back({"heart"					, guiV.shader	, guiF.shader			, "resources/texture/heart.png"});
		materials.push_back({"shop_bg"					, guiV.shader	, guiF.shader			, "resources/texture/shop_bg.png"});
		materials.push_back({"button"					, guiV.shader	, guiF.shader			, "resources/texture/button.png"});
		materials.push_back({"button_disabled"			, guiV.shader	, guiGrayscaleF.shader	, "resources/texture/button.png"});
		materials.push_back({"empty"					, solidV.shader	, emptyF.shader			, "resources/texture/dirt.png"});
		materials.push_back({"gui_empty"				, guiV.shader	, emptyF.shader			, "resources/texture/dirt.png"});
		
		materials.push_back({"gui_font"				    , fontV.shader , guiF.shader			, "resources/texture/font.png"});

		// create index buffers
		for (int i = 0; i < (int)materials.size(); i++) {
			materialIndices.push_back({});
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

		addScreenRect(-50000.f, -10000.f, -5000.f, 100000.f, 20000.f, getMatID("sky"));
		for (int x = -100; x < 100; x++) {
			addRect((float)x * 150.f, 0.f, -15.f, 150.f, 30.f, getMatID("hills"));
			addRect((float)x * 150.f, -300.f, -15.f, 150.f, 300.f, getMatID("stone_background"));
		}
		for (int i = 0; i < (int)game->world.clouds.size(); i++) {
			addRect(game->world.clouds.at(i).r, game->world.clouds.at(i).g, game->world.clouds.at(i).b, 150.f, 75.f, getMatID("cloud"));
		}
		for (int i = 0; i < (int)game->world.sections.size(); i++) {
			if (
				(float)(game->world.sections.at(i).x * SECT_SIZE) > game->world.camera.right(0.f) ||
				(float)((game->world.sections.at(i).x + 1) * SECT_SIZE) < game->world.camera.left(0.f) ||
				(float)(game->world.sections.at(i).y * SECT_SIZE) > game->world.camera.top(0.f) ||
				(float)((game->world.sections.at(i).y + 1) * SECT_SIZE) < game->world.camera.bottom(0.f)
			) continue;
			for (int x = 0; x < SECT_SIZE; x++) {
				if ((float)x + 1.f + (float)(game->world.sections.at(i).x * SECT_SIZE) < game->world.camera.left(0.f) - 2.f) continue;
				if ((float)x + (float)(game->world.sections.at(i).x * SECT_SIZE) > game->world.camera.right(0.f) + 2.f) break;
				//addRect(game->world.sections.at(i).x * SECT_SIZE + x, game->world.gen[2].getVal((float)(game->world.sections.at(i).x * SECT_SIZE + x) / 5.f), 0.08f, 1.f, 0.2f, getMatID("sky"));
				for (int y = 0; y < SECT_SIZE; y++) {
					if ((float)y + 1.f + (float)(game->world.sections.at(i).y * SECT_SIZE) < game->world.camera.bottom(0.f) - 2.f) continue;
					if ((float)y + (float)(game->world.sections.at(i).y * SECT_SIZE) > game->world.camera.top(0.f) + 2.f) break;
					Vec3 lightColor = game->world.sections.at(i).lightmap[x][y];
					addTile(game->world.sections.at(i).x * SECT_SIZE + x, game->world.sections.at(i).y * SECT_SIZE + y, 0.f,  game->world.sections.at(i).tiles[x][y].type, lightColor.r, lightColor.g, lightColor.b, false);
					addTile(game->world.sections.at(i).x * SECT_SIZE + x, game->world.sections.at(i).y * SECT_SIZE + y, -2.f, game->world.sections.at(i).bgTiles[x][y].type, lightColor.r, lightColor.g, lightColor.b, true);
					if (game->world.sections.at(i).tiles[x][y].health < game->world.sections.at(i).tiles[x][y].maxHealth) {
						addRect((float)(game->world.sections.at(i).x * SECT_SIZE + x), (float)(game->world.sections.at(i).y * SECT_SIZE + y), 0.001f, 1.f, 1.f, getMatID("tile_cracks"), floor((game->world.sections.at(i).tiles[x][y].maxHealth - game->world.sections.at(i).tiles[x][y].health) / game->world.sections.at(i).tiles[x][y].maxHealth * 8.f) / 8.f, 0.f, 1.f / 8.f, 1.f);
					}
				}
			}
			if (debug) addRect(game->world.sections.at(i).x * SECT_SIZE, game->world.sections.at(i).y * SECT_SIZE, 0.05f, SECT_SIZE, SECT_SIZE, getMatID("outline"));
		}
		addRect(floor(controls.worldMouse.x), floor(controls.worldMouse.y), 0.002f, 1.f, 1.f, getMatID("select"));
		for (int i = 0; i < (int)game->world.entities.size(); i++) {
			Entity* e = game->world.entities.at(i);
			if (e->type == etypes::item) {
				float heightOffset = sin(game->world.time * 3.f) * 0.05f;
				addRotatedRect(e->pos.x - e->items.at(e->itemNumber).size.x / 4.f, e->pos.y + heightOffset, 0.003f, e->items.at(e->itemNumber).size.x / 2.f, e->items.at(e->itemNumber).size.y / 2.f, getMatID(e->items.at(e->itemNumber).material), sin(game->world.time * 4.f) * 0.03f, e->pos.x, e->pos.y + e->items.at(e->itemNumber).size.y / 4.f + heightOffset);
			} else if (e->type == etypes::player) {
				int fvx = game->world.entities.at(i)->facingVector.x;
				//addRect(e->pos.x - e->size.x / 2.f, e->pos.y, 0.f, e->size.x, e->size.y, getMatID(e->material), fvx == -1 ? 1.f : 0.f, 0.f, fvx == -1 ? -1.f : 1.f, 1.f);
				float bodyX = e->pos.x - e->size.x / 2.f;
				float bodyY = e->pos.y + e->size.y / 3.f;
				float bodyWidth = e->size.x;
				float bodyHeight = e->size.y / 3.f;
				float leftLegX = e->pos.x - e->size.x / 2.f;
				float rightLegX = e->pos.x + e->size.x / 6.f;
				float legBottomY = e->pos.y;
				float legTopY = e->pos.y + e->size.y / 6.f;
				float legWidth = e->size.x / 3.f;
				float legHeight = e->size.y / 6.f;
				addRect(bodyX, bodyY, 0.f, bodyWidth, bodyHeight, getMatID("skin_body_gregger"));
				float theta = e->pos.x * 2.5;
				float leftLegBottomTopX = leftLegX + legWidth / 2.f + sin(theta) * legHeight;
				float leftLegBottomTopY = legTopY + legHeight + cos(theta) * legHeight;
				float rightLegBottomTopX = rightLegX + legWidth / 2.f + sin(PI + theta) * legHeight;
				float rightLegBottomTopY = legTopY + legHeight + cos(PI + theta) * legHeight;
				addRotatedRect(leftLegX, legTopY, 0.f, legWidth, legHeight, getMatID("stone"), theta + PI, leftLegX + legWidth / 2.f, legTopY + legHeight);
				addRotatedRect(leftLegBottomTopX - legWidth / 2.f, leftLegBottomTopY - legHeight, 0.f, legWidth, legHeight, getMatID("stone"), game->world.time * 0.f, leftLegX + legWidth / 2.f, legBottomY + legHeight);
				addRotatedRect(rightLegX, legTopY, 0.f, legWidth, legHeight, getMatID("stone"), theta, rightLegX + legWidth / 2.f, legTopY + legHeight);
				addRotatedRect(rightLegBottomTopX - legWidth / 2.f, rightLegBottomTopY - legHeight, 0.f, legWidth, legHeight, getMatID("stone"), game->world.time * 0.f, rightLegX + legWidth / 2.f, legBottomY + legHeight);
				float handX = (fvx == -1) ? (e->pos.x - e->size.x / 2.f - 0.1f) : (e->pos.x + e->size.x / 2.f + 0.1f);
				addRotatedRect(handX, e->pos.y + e->size.y / 2.f + 0.1f, 0.003f, e->items.at(e->itemNumber).size.x * (fvx == -1 ? -1.f : 1.f), e->items.at(e->itemNumber).size.y, getMatID(e->items.at(e->itemNumber).material), e->swingRotation * ((e->facingVector.x < 0) ? -1.f : 1.f), handX, e->pos.y + e->size.y / 2.f + 0.1f);
			} else {
				addRect(e->pos.x, e->pos.y, 0.f, e->size.x, e->size.y, getMatID(e->material));
			}
		}
		for (int i = 0; i < (int)game->world.particles.size(); i++) {
			float size = game->world.particles.at(i).time;
			addRect(game->world.particles.at(i).pos.x - game->world.particles.at(i).size.x / 2.f * size, game->world.particles.at(i).pos.y - game->world.particles.at(i).size.y / 2.f * size, 0.004f, game->world.particles.at(i).size.x * size, game->world.particles.at(i).size.y * size, getMatID(game->world.particles.at(i).material));
		}
		/*for (int i = 0; i < (int)game->world.photons.size(); i++) {
			addTri(game->world.photons.at(i).x, game->world.photons.at(i).y, 0.005f, 0.1f, getMatID("snow"));
		}*/

		float redness = game->world.entities.at(game->world.self)->damageRedness;
		float vignetteSize = lerp(5.f, 1.f, redness);
		if (redness > 0.f) addScreenRect(-vignetteSize, -vignetteSize, -0.05f, vignetteSize * 2.f, vignetteSize * 2.f, getMatID("blood_vignette"));
		for (int i = 0; i < (int)game->world.entities.at(game->world.self)->items.size(); i++) {
			addScreenRect((float)i * 0.1f - 0.45f, -1.f, -0.099f, 0.1f, 0.1f * aspect, getMatID("items_slot"));
			addScreenRect((float)i * 0.1f - 0.44f, -0.99f, -0.1f, 0.08f, 0.08f * aspect, getMatID(game->world.entities.at(game->world.self)->items.at(i).material + "_gui"));
			addScreenRect((float)i * 0.1f - 0.45f, -0.99f, -0.101f, 0.095f, 0.01f, getMatID("health_bg"));
			addScreenRect((float)i * 0.1f - 0.45f, -0.99f, -0.102f, 0.095f * game->world.entities.at(game->world.self)->items.at(i).durability / game->world.entities.at(game->world.self)->items.at(i).maxDurability, 0.01f, getMatID("health_green"));
		}
		addScreenRect((float)game->world.entities.at(game->world.self)->itemNumber * 0.1f - 0.45f, -1.f, -0.11f, 0.1f, 0.1f * aspect, getMatID("items_selected"));
		float healthRatio = game->world.entities.at(game->world.self)->health / game->world.entities.at(game->world.self)->maxHealth;
		float bottom = 1.f - 0.1f * aspect;
		float top = 1.f - 0.02f * aspect;
		addScreenQuad(-0.4f, top, 0.6f, top, -0.5f, bottom, 0.5f, bottom, -0.105f, getMatID("health_bg"));
		addScreenQuad(-0.4f, top, healthRatio - 0.4f, top, -0.5f, bottom, healthRatio - 0.5f, bottom, -0.11f, getMatID("health_green"));
		addScreenRect(-0.55f, 1.f - 0.15f * aspect, -0.115f, 0.15f, 0.15f * aspect, getMatID("heart"));

		for (int i = 0; i < (int)buttons.size(); i++) {
			if (!buttons.at(i).visible) continue;
			addScreenRect(buttons.at(i).x, buttons.at(i).y, -0.11f, buttons.at(i).width, buttons.at(i).height, getMatID(buttons.at(i).enabled ? "button" : "button_disabled"));
			addTextLine(buttons.at(i).text, buttons.at(i).x + buttons.at(i).width / 2.f, buttons.at(i).y + buttons.at(i).height / 2.f - 0.01f, -0.12f, 0.02f, 0.8f, true);
		}
		if (game->shopOpen) {
			addScreenRect(-0.8f, -0.8f, -0.09f, 1.6f, 1.6f, getMatID("shop_bg"));
			addScreenRect(-0.8f, -0.8f, -0.1f, 0.1f, 0.2f, getMatID(game->world.entities.at(game->world.self)->material + "_gui"));
			addTextLine("SHOP", 0.f, 0.7f, -0.1f, 0.1f, 1.f, true);
		}
			if (game->commandPromptOpen) {
			addScreenRect(-1.f, -0.8f, -0.09f, 2.f, 0.2f, getMatID("button"));
			addTextLine(game->commandPromptText, -0.8f, -0.7f, -0.1f, 0.05f, 0.8f, false);
		}
		addTextLine(ftos(game->world.time / 600.f, 5), 0.f, -0.7f, -0.1f, 0.05f, 0.8f, true);

		if (debug) {
			addText(to_string(fps) + " FPS", -0.9f, 0.9f, -0.1f, 0.05f, 0.8f, 2.f, false);
			addText(string("X ") + ftos(game->world.entities.at(game->world.self)->pos.x, 5), -0.9f, 0.75f, -0.1f, 0.05f, 0.8f, 2.f, false);
			addText(string("Y ") + ftos(game->world.entities.at(game->world.self)->pos.y, 5), -0.9f, 0.6f, -0.1f, 0.05f, 0.8f, 2.f, false);
			addText(string("XV ") + ftos(game->world.entities.at(game->world.self)->vel.x, 5), -0.9f, 0.45f, -0.1f, 0.05f, 0.8f, 2.f, false);
			addText(string("YV ") + ftos(game->world.entities.at(game->world.self)->vel.y, 5), -0.9f, 0.3f, -0.1f, 0.05f, 0.8f, 2.f, false);
		}
		int tris = 0;
		for (int i = 0; i < (int)materialIndices.size(); i++) {
			tris += (int)materialIndices.at(i).size() / 3;
		}
		//addText("tris: " + to_string(tris), -0.9f, 0.1f, -0.1f, 0.05f, 0.8f, 2.f, false);
		//addText("photons: " + to_string(game->world.photons.size()), -0.9f, -0.05f, -0.1f, 0.05f, 0.8f, 2.f, false);
		//addText("block size: " + to_string((long long)((float)height / game->world.camera.zoom / 2.f)) + "px", -0.9f, -0.2f, -0.1f, 0.05f, 0.8f, 2.f, false);
	}
	void renderMaterials() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, width, height);
		buildThem();
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), &vertices[0], GL_STATIC_DRAW);
		for (int i = 0; i < (int)materials.size(); i++) {
			if ((int)materialIndices.at(i).size() > 0) renderMaterial(i);
		}

	}
	void renderMaterial(int id) {


		glEnableVertexAttribArray(materials.at(id).vpos_location);
		glVertexAttribPointer(materials.at(id).vpos_location, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (void*)0);
	
		glEnableVertexAttribArray(materials.at(id).vtexcoord_location);
		glVertexAttribPointer(materials.at(id).vtexcoord_location, 2, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (void*)(sizeof(float) * 3));
	
		glEnableVertexAttribArray(materials.at(id).vtilehealth_location);
		glVertexAttribPointer(materials.at(id).vtilehealth_location, 1, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (void*)(sizeof(float) * 5));
	
		glEnableVertexAttribArray(materials.at(id).vlightcolor_location);
		glVertexAttribPointer(materials.at(id).vlightcolor_location, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (void*)(sizeof(float) * 6));

		float ratio = (float)width / (float)height;
		mat4x4 m, p, mvp;

		glBufferData(GL_ELEMENT_ARRAY_BUFFER, materialIndices.at(id).size() * sizeof(unsigned int), &materialIndices.at(id)[0], GL_STATIC_DRAW);
		
		glBindTexture(GL_TEXTURE_2D, materials.at(id).texture);

		mat4x4_identity(m);
		//mat4x4_scale_aniso(m, m, ratio, 1.f, 1.f);
		mat4x4_translate(m, -game->world.camera.pos.x, -game->world.camera.pos.y, -game->world.camera.zoom);
		/*mat4x4_rotate_X(m, m, game->world.camera.rotation.x);
		mat4x4_rotate_Y(m, m, game->world.camera.rotation.y);
		mat4x4_rotate_Z(m, m, game->world.camera.rotation.z);*/
		//mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
		mat4x4_perspective(p, 1.57f, ratio, 0.01f, 100000.f);
		mat4x4_mul(mvp, p, m);

		glUseProgram(materials.at(id).program);

		glUniform1i(materials.at(id).texture1_location, 0);
		glUniform1f(materials.at(id).time_location, game->world.time);
		glUniformMatrix4fv(materials.at(id).mvp_location, 1, GL_FALSE, (const GLfloat*)mvp);
		glDrawElements(GL_TRIANGLES, materialIndices.at(id).size(), GL_UNSIGNED_INT, (void*)0);
	}
private:
	void clearVertices() {
		vertices.clear();
		for (int i = 0; i < (int)materialIndices.size(); i++) {
			materialIndices.at(i).clear();
		}
	}

	void addTri(float x, float y, float z, float size, int matId) {
		if (x < game->world.camera.left(z) || x > game->world.camera.right(z) || y < game->world.camera.bottom(z) || y > game->world.camera.top(z)) return;
		unsigned int end = vertices.size();
		materialIndices.at(matId).insert(materialIndices.at(matId).end(), {
			0U+end, 1U+end, 2U+end
		});
		vertices.insert(vertices.end(), {
			{x+0.8660254f * size, y - 0.5f * size, z, 1.f, -.5f, 1.f, 1.f, 1.f, 1.f},
			{x-0.8660254f * size, y - 0.5f * size, z, -1.f, -.5f, 1.f, 1.f, 1.f, 1.f},
			{x                  , y + size       , z, 0.f, 1.f, 1.f, 1.f, 1.f, 1.f}
		});
	}
	void addRect(float x, float y, float z, float w, float h, int matId, float u = 0.f, float v = 0.f, float s = 1.f, float t = 1.f) {
		if (x + w < game->world.camera.left(z) || x > game->world.camera.right(z) || y + h < game->world.camera.bottom(z) || y > game->world.camera.top(z)) return;
		unsigned int end = vertices.size();
		materialIndices.at(matId).insert(materialIndices.at(matId).end(), {
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
	void addScreenQuad(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float z, int matId) {
		unsigned int end = vertices.size();
		materialIndices.at(matId).insert(materialIndices.at(matId).end(), {
			0U+end, 2U+end, 1U+end,
			1U+end, 2U+end, 3U+end
		});
		vertices.insert(vertices.end(), {
			{x1, y1, z, 0.f, 0.f, 1.f, 1.f, 1.f, 1.f},
			{x2, y2, z, 1.f, 0.f, 1.f, 1.f, 1.f, 1.f},
			{x3, y3, z, 0.f, 1.f, 1.f, 1.f, 1.f, 1.f},
			{x4, y4, z, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f}
		});
	}
	void addScreenRect(float x, float y, float z, float w, float h, int matId, float u = 0.f, float v = 0.f, float s = 1.f, float t = 1.f) {
		unsigned int end = vertices.size();
		materialIndices.at(matId).insert(materialIndices.at(matId).end(), {
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
	void addWorldRect(float x, float y, float z, float w, float h, int matId, float tileHealth, float lightR, float lightG, float lightB, float u=0.f, float v=0.f, float s=1.f, float t=1.f) {
		if (x + w < game->world.camera.left(z) + z || x > game->world.camera.right(z) - z || y + h < game->world.camera.bottom(z) + z || y > game->world.camera.top(z) - z) return;
		unsigned int end = vertices.size();
		materialIndices.at(matId).insert(materialIndices.at(matId).end(), {
			0U+end, 2U+end, 1U+end,
			1U+end, 2U+end, 3U+end
		});
		vertices.insert(vertices.end(), {
			{x  , y+h, z, u, v, tileHealth, lightR, lightG, lightB},
			{x+w, y+h, z, u+s, v, tileHealth, lightR, lightG, lightB},
			{x  , y  , z, u, v+t, tileHealth, lightR, lightG, lightB},
			{x+w, y  , z, u+s, v+t, tileHealth, lightR, lightG, lightB}
		});
	}
	void addTile(int x, int y, int z, int type, float lightR, float lightG, float lightB, bool isBackground) {
		float uvSize;
		switch (type) {
			case ttypes::air:
			//addWorldRect((float)x, (float)y, (float)z, 1.f, 1.f, getMatID("air"), 1.f, lightR, lightG, lightB);
			break;
			case ttypes::dirt:
			if ((isBackground ? game->world.getBgTile(x, y + 1) : game->world.getTile(x, y + 1)).type == ttypes::air) {
				uvSize = 10.f;
				addWorldRect((float)x, (float)y, (float)z, 1.f, 1.f, getMatID("grass"), 1.f, lightR, lightG, lightB, (float)x / uvSize, (float)y / uvSize, 1.f / uvSize, 1.f / uvSize);
			} else {
				uvSize = 5.f;
				addWorldRect((float)x, (float)y, (float)z, 1.f, 1.f, getMatID("dirt"), 1.f, lightR, lightG, lightB, (float)x / uvSize, (float)y / uvSize, 1.f / uvSize, 1.f / uvSize);
			}
			break;
			case ttypes::snow:
			addWorldRect((float)x, (float)y, (float)z, 1.f, 1.f, getMatID("snow"), 1.f, lightR, lightG, lightB);
			break;
			case ttypes::stone:
			uvSize = 5.f;
			addWorldRect((float)x, (float)y, (float)z, 1.f, 1.f, getMatID("stone"), 1.f, lightR, lightG, lightB, (float)x / uvSize, (float)y / uvSize, 1.f / uvSize, 1.f / uvSize);
			break;
			case ttypes::wood:
			addWorldRect((float)x, (float)y, (float)z, 1.f, 1.f, getMatID("wood"), 1.f, lightR, lightG, lightB);
			break;
			case ttypes::log:
			addWorldRect((float)x, (float)y, (float)z, 1.f, 1.f, getMatID("log"), 1.f, lightR, lightG, lightB);
			break;
			case ttypes::leaves:
			addWorldRect((float)x, (float)y, (float)z, 1.f, 1.f, getMatID("leaves"), 1.f, lightR, lightG, lightB);
			break;
			case ttypes::sentry_shack_bottom:
			addWorldRect((float)x, (float)y, (float)z, 1.f, 1.f, getMatID("sentry_shack_bottom"), 1.f, lightR, lightG, lightB);
			break;
			case ttypes::sentry_shack_middle:
			addWorldRect((float)x, (float)y, (float)z, 1.f, 1.f, getMatID("sentry_shack_middle"), 1.f, lightR, lightG, lightB);
			break;
			case ttypes::sentry_shack_top:
			addWorldRect((float)x, (float)y, (float)z, 1.f, 1.f, getMatID("sentry_shack_top"), 1.f, lightR, lightG, lightB);
			break;
		}
	}
	void addRotatedRect(float x, float y, float z, float w, float h, int matId, float theta, float originX, float originY) {
		if (x + w < game->world.camera.left(z) || x > game->world.camera.right(z) || y + h < game->world.camera.bottom(z) || y > game->world.camera.top(z)) return;
		unsigned int end = vertices.size();
		materialIndices.at(matId).insert(materialIndices.at(matId).end(), {
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
	void addCharacter(char character, float x, float y, float z, float w) {
		Vec2 uv = getCharacterCoords(character);
		unsigned int end = vertices.size();
		materialIndices.at(getMatID("gui_font")).insert(materialIndices.at(getMatID("gui_font")).end(), {
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
			z -= 0.001f;
		}
	}
	void addTextLine(string text, float x, float y, float z, float w, float spacing, bool centered) {
		for (int i = 0; i < (int)text.length(); i++) {
			float noWrapX = (float)i * w * spacing;
			if (centered) noWrapX -= (float)text.length() * w * spacing / 2.f;
			addCharacter(text.at(i), noWrapX + x, y - w * 1.6f, z, w * 1.5f);
			z -= 0.001f;
		}
	}
};

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
	glEnable(GL_DEPTH_TEST);
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
		glDeleteTextures(1, &renderer.materials.at(i).texture);
	}
	soundDoer.exit();
	glfwDestroyWindow(window);

	glfwTerminate();
	exit(EXIT_SUCCESS);
}
