namespace Chave {

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
		Shader worldUvRandomF{"resources/shader/world_uv_random.fsh", GL_FRAGMENT_SHADER};
		Shader worldUvF{"resources/shader/world_uv.fsh", GL_FRAGMENT_SHADER};
		Shader guiGrayscaleF{"resources/shader/gui_grayscale.fsh", GL_FRAGMENT_SHADER};
		Shader rangeBlueF{"resources/shader/range_blue.fsh", GL_FRAGMENT_SHADER};
		Shader rangeGreenF{"resources/shader/range_green.fsh", GL_FRAGMENT_SHADER};
		Shader rangeRedF{"resources/shader/range_red.fsh", GL_FRAGMENT_SHADER};
		Shader electricityF{"resources/shader/electricity.fsh", GL_FRAGMENT_SHADER};
		Shader worldUvRandomXyF{"resources/shader/world_uv_random_xy.fsh", GL_FRAGMENT_SHADER};
		Shader worldUvRandomYzF{"resources/shader/world_uv_random_yz.fsh", GL_FRAGMENT_SHADER};

		vector<Material> materials = {
			{"solid"					  , solidV.shader, solidF.shader		  , "resources/texture/brain.png"},
			{"gui"						, guiV.shader  , guiF.shader			, "resources/texture/screen.png"},
			{"grass"					  , solidV.shader, worldUvRandomF.shader  , "resources/texture/grass.png"},
			{"stone"					  , solidV.shader, solidF.shader		  , "resources/texture/stone.png"},
			{"wood"					   , solidV.shader, solidF.shader		  , "resources/texture/wood.png"}, 
			{"skin"					   , solidV.shader, solidF.shader		  , "resources/texture/skin.png"}, 
			{"metal"					  , solidV.shader, solidF.shader		  , "resources/texture/metal.png"}, 
			{"gold"					   , solidV.shader, solidF.shader		  , "resources/texture/gold.png"}, 
			{"camo"					   , solidV.shader, solidF.shader		  , "resources/texture/camo.png"}, 
			{"rock"					   , solidV.shader, solidF.shader		  , "resources/texture/rock.png"}, 
			{"metal_black"				, solidV.shader, solidF.shader		  , "resources/texture/metal_black.png"}, 
			{"straw"					  , solidV.shader, solidF.shader		  , "resources/texture/straw.png"}, 

			{"fire"					   , solidV.shader, solidF.shader		  , "resources/texture/fire.png"}, 
			{"electricity"				, solidV.shader, electricityF.shader	, "resources/texture/icon_battery.png"},
			{"rock_world_uv_random_xy"	, solidV.shader, worldUvRandomXyF.shader, "resources/texture/rock.png"}, 
			{"rock_world_uv_random_yz"	, solidV.shader, worldUvRandomYzF.shader, "resources/texture/rock.png"}, 

			{"range_blue"				 , solidV.shader, rangeBlueF.shader	  , "resources/texture/brain.png"}, 
			{"range_green"				, solidV.shader, rangeGreenF.shader	 , "resources/texture/brain.png"}, 
			{"range_red"				  , solidV.shader, rangeRedF.shader	   , "resources/texture/brain.png"}, 

			{"gui_container"			  , guiV.shader  , guiF.shader			, "resources/texture/gui_container.png"}, 
			{"gui_button"				 , guiV.shader  , guiF.shader			, "resources/texture/button.png"}, 
			{"gui_button_disabled"		, guiV.shader  , guiGrayscaleF.shader   , "resources/texture/button.png"}, 
			{"gui_icon_archer"			, guiV.shader  , guiF.shader			, "resources/texture/icon_archer.png"}, 
			{"gui_icon_cannon"			, guiV.shader  , guiF.shader			, "resources/texture/icon_cannon.png"}, 
			{"gui_icon_turret"			, guiV.shader  , guiF.shader			, "resources/texture/icon_turret.png"}, 
			{"gui_icon_tank"			  , guiV.shader  , guiF.shader			, "resources/texture/icon_tank.png"}, 
			{"gui_icon_gold_mine"		 , guiV.shader  , guiF.shader			, "resources/texture/icon_gold_mine.png"}, 
			{"gui_icon_battery"		   , guiV.shader  , guiF.shader			, "resources/texture/icon_battery.png"},
			{"gui_icon_scientist"		 , guiV.shader  , guiF.shader,			 "resources/texture/icon_scientist.png"}, 
			{"gui_icon_robot"			 , guiV.shader  , guiF.shader,			 "resources/texture/icon_robot.png"}, 
			{"gui_icon_scarecrow"		 , guiV.shader  , guiF.shader,			 "resources/texture/icon_scarecrow.png"}, 
			{"gui_button_upgrade"		 , guiV.shader  , guiF.shader			, "resources/texture/button_upgrade.png"}, 
			{"gui_button_upgrade_disabled", guiV.shader  , guiGrayscaleF.shader   , "resources/texture/button_upgrade.png"}, 
			{"gui_play"				   , guiV.shader  , guiF.shader			, "resources/texture/button_play.png"}, 
			{"gui_play_disabled"		  , guiV.shader  , guiGrayscaleF.shader   , "resources/texture/button_play.png"}, 
			{"icon_level"				 , guiV.shader  , guiF.shader			, "resources/texture/icon_level.png"}, 
			{"gui_font"				   , fontV.shader , guiF.shader			, "resources/texture/font.png"}, 
		};

		vector<Vertex> vertices = {};
		vector<vector<unsigned int>> indiceses = {};
		
		Mesh turretMesh{&materials, "resources/model/person/turret.obj"};
		Mesh cannonMesh{&materials, "resources/model/person/cannon.obj"};
		Mesh archerMesh{&materials, "resources/model/person/archer.obj"};
		Mesh tankMesh{&materials, "resources/model/person/tank.obj"};
		Mesh goldMineMesh{&materials, "resources/model/person/goldmine.obj"};
		Mesh batteryMesh{&materials, "resources/model/person/battery.obj"};
		Mesh scientistMesh{&materials, "resources/model/person/scientist.obj"};
		Mesh robotMesh{&materials, "resources/model/person/robot.obj"};
		Mesh scarecrowMesh{&materials, "resources/model/person/scarecrow.obj"};

		Mesh entityTemplateMesh{&materials, "resources/model/entity/normal.obj"};
		Mesh normalMesh{&materials, "resources/model/entity/normal.obj"};
		Mesh fastMesh{&materials, "resources/model/entity/fast.obj"};
		Mesh monsterMesh{&materials, "resources/model/entity/monster.obj"};
		Mesh generalMesh{&materials, "resources/model/entity/general.obj"};
		Mesh ironMaidenMesh{&materials, "resources/model/entity/ironmaiden.obj"};
		Mesh tungstenMaidenMesh{&materials, "resources/model/entity/tungstenmaiden.obj"};
		Mesh twinMesh{&materials, "resources/model/entity/twin.obj"};

		Mesh missileMesh{&materials, "resources/model/projectile/missile.obj"};
		Mesh cannonballMesh{&materials, "resources/model/projectile/cannonball.obj"};
		Mesh sparkMesh{&materials, "resources/model/projectile/spark.obj"};
		Mesh electricityMesh{&materials, "resources/model/projectile/electricity.obj"};
		Mesh potionMesh{&materials, "resources/model/projectile/potion.obj"};

		Mesh planeMesh{&materials, "resources/model/plane.obj"};

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

			if (game->gameStatus == 0) {
				// ground
				for (int x = -10; x < game->world.level.size.x + 10; x++) {
					for (int z = -10; z < game->world.level.size.z + 10; z++) {
						if (x < 0 || z < 0 || x >= game->world.level.size.x || z >= game->world.level.size.z) {
							addPlane((float)x, game->world.level.size.y, (float)z, 1.f, 1.f, false, getMatID(game->world.level.groundMaterial));
						}
					}
				}
				float sx = game->world.level.size.x;
				float sy = game->world.level.size.y;
				float sz = game->world.level.size.z;
				addQuad({0.f, sy , 0.f}, {0.f, sy, sz }, {0.f, 0.f, sz }, {0.f, 0.f, 0.f}, getMatID(game->world.level.wallMaterial));
				addQuad({sx , sy , sz }, {sx , sy, 0.f}, {sx , 0.f, 0.f}, {sx , 0.f, sz }, getMatID(game->world.level.wallMaterial));
				addQuad({sx , sy, 0.f}, {0.f, sy , 0.f}, {0.f, 0.f, 0.f}, {sx , 0.f, 0.f}, getMatID(game->world.level.wallMaterial));
				addQuad({0.f, sy, sz }, {sx , sy , sz }, {sx , 0.f, sz }, {0.f, 0.f, sz }, getMatID(game->world.level.wallMaterial));
				addPlane(0.f, 0.f, 0.f, game->world.level.size.x, game->world.level.size.z, true, getMatID(game->world.level.groundMaterial));
				// point path
				for (int i = 0; i < (int)game->world.level.path.size(); i++) {
					if (i < (int)game->world.level.path.size() - 1) {
						addPath(game->world.level.path[i], game->world.level.path[i + 1], game->world.level.pathWidth + 0.1f, 0.01f, getMatID(game->world.level.pathBaseMaterial));
						addPath(game->world.level.path[i], game->world.level.path[i + 1], game->world.level.pathWidth, 0.02f, getMatID(game->world.level.pathMaterial));
					}
					addDisc(vec3Add(game->world.level.path[i], {0.f, 0.01f, 0.f}), (game->world.level.pathWidth + 0.1f) / 2.f, 15, getMatID(game->world.level.pathBaseMaterial));
					addDisc(vec3Add(game->world.level.path[i], {0.f, 0.02f, 0.f}), game->world.level.pathWidth / 2.f, 15, getMatID(game->world.level.pathMaterial));
				}
				// entities
				for (Entity entity : game->world.entities) {
					switch (entity.type) {
					case ENTITY_ENTITYTEMPLATE:
						addMesh(entityTemplateMesh, entity.pos, {.5f, .8f, .5f}, entity.yRotation);
						break;
					case ENTITY_NORMAL:
						addMesh(normalMesh, entity.pos, {.5f, .8f, .5f}, entity.yRotation);
						break;
					case ENTITY_FAST:
						addMesh(fastMesh, entity.pos, {.5f, .8f, .5f}, entity.yRotation);
						break;
					case ENTITY_MONSTER:
						addMesh(monsterMesh, entity.pos, {.5f, .8f, .5f}, entity.yRotation);
						break;
					case ENTITY_GENERAL:
						addMesh(generalMesh, entity.pos, {1.f, 1.f, 1.f}, entity.yRotation);
						break;
					case ENTITY_IRON_MAIDEN:
						addMesh(ironMaidenMesh, entity.pos, {1.f, 1.f, 1.f}, entity.yRotation);
						break;
					case ENTITY_TUNGSTEN_MAIDEN:
						addMesh(tungstenMaidenMesh, entity.pos, {1.f, 1.f, 1.f}, entity.yRotation);
						break;
					case ENTITY_TWIN:
						addMesh(twinMesh, entity.pos, {1.f, 1.f, 1.f}, entity.yRotation);
						break;
					default:
						addCube(entity.pos.x - entity.size.x / 2.f, entity.pos.y, entity.pos.z - entity.size.z / 2.f, entity.size.x, entity.size.y, entity.size.z, 0);
						break;
					}
					addPlane(entity.pos.x, entity.pos.y + entity.size.y + 0.25f, entity.pos.z - 0.5f, 0.2f, 1.f, false, getMatID("metal_black"));
					
					addPlane(entity.pos.x, entity.pos.y + entity.size.y + 0.26f, entity.pos.z - 0.5f, 0.2f, entity.health / entity.maxHealth, false, getMatID("gold"));
				}
				//people
				for (Person person : game->world.people) {
					addPerson(&person, false);
				}
				if (game->isPlacingPerson) {
					addPerson(&game->placingPerson, true);
				}
				//projectiles
				for (Projectile projectile : game->world.projectiles) {
					switch (projectile.type) {
					case PROJECTILE_MISSILE:
						addMesh(missileMesh, projectile.pos, {1.f, 1.f, 1.f}, atan2(projectile.velocity.z, projectile.velocity.x));
						break;
					case PROJECTILE_SPARK:
						addMesh(sparkMesh, vec3Add(projectile.pos, projectile.velocity.normalise(0.7f)), {1.f, 1.f, 1.f}, atan2(projectile.velocity.z, projectile.velocity.x));
						break;
					case PROJECTILE_CANNONBALL:
						addMesh(cannonballMesh, projectile.pos, {1.f, 1.f, 1.f}, atan2(projectile.velocity.z, projectile.velocity.x));
						break;
					case PROJECTILE_ELECTRICITY:
						addArc(projectile.origin, projectile.pos);
						break;
					case PROJECTILE_POTION:
						addMesh(potionMesh, projectile.pos, {1.f, 1.f, 1.f}, atan2(projectile.velocity.z, projectile.velocity.x));
						break;
					default:
						addPath(projectile.pos, vec3Add(projectile.pos, projectile.velocity), 0.1f, 0.f, 1);
					}
				}


				// clip space: smaller z is in front

				addText("HEALTH: " + to_string((int)game->world.health), -0.67f, 0.8f, 0.1f, 0.07f, 0.8f, 2.f, false);
				addText("WAVE " + to_string(game->world.waveNumber), -0.3f, -0.95f, 0.1f, 0.07f, 0.8f, 2.f, false);
				addText("$" + commas(game->world.money), -0.25f, 0.5f, 0.1f, 0.07f, 0.8f, 14.f * 0.07f * 0.8f, false);
				addText("fps:" + ftos(1.f / (float)(frameTime), 2), 0.f, 0.8f, 0.1f, 0.07f, 0.8f, 2.f, false);
				if (debug) {
					addText("ents:" + to_string((int)game->world.entities.size()), -0.7f, 0.7f, 0.1f, 0.07f, 0.8f, 2.f, false);
					addText("prjs:" + to_string((int)game->world.projectiles.size()), -0.7f, 0.55f, 0.1f, 0.07f, 0.8f, 2.f, false);
					addText("snds:" + to_string((int)soundDoer.buffers.size()), -0.7f, 0.4f, 0.1f, 0.07f, 0.8f, 2.f, false);
				}
				/*if (game->world.waveNumber == game->waves.size() && game->waveEnded() && game->health > 0.f) {
					addText("YOU WON", -0.9f, -0.9f, 0.1f, 0.2f, 0.8f);
				}*/
				addRect(0.7f, -1.f, 0.25f, 0.3f, 2.f, getMatID("gui_container")); // right
				addRect(0.725f, -0.95f, 0.2f, 0.25f, 0.25f, game->waveEnded() ? getMatID("gui_play") : getMatID("gui_play_disabled")); // play button
				for (Person person : game->world.people) {
					if (person.selected) {
						addRect(0.75f, 0.55f, 0.2f, 0.15f, 0.15f, (person.stats.level < (int)person.getUpgrades().size() - 1 && game->world.money >= person.getUpgrades()[person.stats.level + 1].price) ? getMatID("gui_button_upgrade") : getMatID("gui_button_upgrade_disabled"));
						vector<PersonUpgrade> upgrades = person.getUpgrades();
						bool isMax = person.stats.level > (int)upgrades.size() - 2;
						string upgradeText = isMax ? "max upgrades" : upgrades[person.stats.level + 1].name + ": $" + commas(upgrades[person.stats.level + 1].price);
						addText(upgradeText, 0.72f, 0.5f, 0.1f, 0.02f, 0.8f, 0.27f, false);
						addRect(0.75f, 0.38f, 0.2f, 0.25f, 0.08f, getMatID("gui_button"));
						addText("sell ($" + commas(person.stats.price * 0.9f) + ")", 0.75f, 0.39f, 0.1f, 0.02f, 0.8f, 0.25f, false);
						float dps = person.stats.projectile.damage / person.stats.shootDelay * person.stats.projectile.health;
						//float rangeDps = dps * (3.14159f * powf(person.stats.range, 2.f));
						addText("dps: " + ftos(dps, 2), 0.72f, 0.32f, 0.1f, 0.02f, 0.8f, 2.f, false);
						addText("dpspp: " + ftos(dps / person.stats.price / (person.stats.size.x * person.stats.size.z), 2), 0.72f, 0.28f, 0.1f, 0.02f, 0.8f, 2.f, false);
						break;
					}
				}
				addRect(-1.f, -1.f, 0.25f, 0.3f, 2.f, getMatID("gui_container")); // people placing buttons
				for (int i = 0; i < (int)personButtons.size(); i++) {
					float x = -0.98f + (0.15f * fmod(i, 2.f));
					float y = 0.7f - floor(i / 2.f) * 0.15f;
					PersonStats personButtonStats = {personButtons[i].type};
					if (personButtons[i].type == PERSON_TANK && !game->world.tankUnlocked) continue;
					addRect(x, y, 0.206f, 0.13f, 0.13f, game->world.money >= personButtonStats.price ? getMatID("gui_button") : getMatID("gui_button_disabled"));
					int matId;
					switch (personButtons[i].type) {
					case PERSON_ARCHER:
						matId = getMatID("gui_icon_archer");
						break;
					case PERSON_CANNON:
						matId = getMatID("gui_icon_cannon");
						break;
					case PERSON_TURRET:
						matId = getMatID("gui_icon_turret");
						break;
					case PERSON_TANK:
						matId = getMatID("gui_icon_tank");
						break;
					case PERSON_GOLD_MINE:
						matId = getMatID("gui_icon_gold_mine");
						break;
					case PERSON_BATTERY:
						matId = getMatID("gui_icon_battery");
						break;
					case PERSON_SCIENTIST:
						matId = getMatID("gui_icon_scientist");
						break;
					case PERSON_ROBOT:
						matId = getMatID("gui_icon_robot");
						break;
					case PERSON_SCARECROW:
						matId = getMatID("gui_icon_scarecrow");
						break;
					}
					if (!game->isPlacingPerson || game->placingPerson.type != personButtons[i].type) {
						addRect(x, y, 0.205f, 0.13f, 0.13f, matId);
					}
					PersonStats stats{personButtons[i].type};
					addText("$" + to_string((long long)stats.price), x, y - 0.02f, 0.1f, 0.03f, 0.7f, 2.f, false);
				}
			} else if (game->gameStatus == 1) {
				addRect(-.9f, -.9f, 0.3f, 1.8f, 1.6f, getMatID("gui_container"));
				addText("LEVEL SELECT", -0.5f, 0.7f, 0.1f, 0.1f, 0.9f, 2.f, false);
				for (int i = 0; i < (int)levels.size(); i++) {
					int x = i % 4;
					int y = (int)floor((float)i / 4.f);
					addRect(-.8f + x * 0.36f, 0.6f - 0.36f * ((float)y + 1.f), 0.29f, 0.36f, 0.36f, getMatID("gui_button"));
					addRect(-.8f + x * 0.36f + 0.06f, 0.6f - 0.36f * ((float)y + 1.f) + 0.06f, 0.28f, 0.36f - 0.12f, 0.36f - 0.12f, getMatID("icon_level"));
					addText(levels[i].name, -.8f + x * 0.36f + 0.29f / 2.f, 0.6f - 0.36f * ((float)y + 1.f), 0.1f, 0.05f, 0.8f, 0.36f, true);
				}
			}
			if (game->messageTime > 0.f) {
				addRect(-0.5f, -0.9f, 0.2f, 1.f, 0.5f, getMatID("gui_container"));
				addText(game->message, -0.45f, -0.6f, 0.1f, 0.07f, 0.7f, 0.9f, false);
			}
			
			if (debug) {
				int tris = 0;
				int verts = (int)vertices.size();
				for (int i = 0; i < (int)indiceses.size(); i++) {
					tris += (int)indiceses[i].size() / 3;
				}
				addText("tris:" + to_string(tris), 0.f, 0.1f, 0.1f, 0.07f, 0.8f, 2.f, false);
				addText("verts:" + to_string(verts), 0.f, 0.0f, 0.1f, 0.07f, 0.8f, 2.f, false);
			}
		}
		void renderMaterials(int width, int height) {
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glViewport(0, 0, width, height);

			buildThem(width, height);
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), &vertices[0], GL_STATIC_DRAW);
			for (int i = 0; i < (int)materials.size(); i++) {
				renderMaterial(i, width, height);
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
			mat4x4_translate(m, -game->world.camera.pos.x, -game->world.camera.pos.y, -game->world.camera.pos.z);
			mat4x4_rotate_X(m, m, game->world.camera.rotation.x);
			mat4x4_rotate_Y(m, m, game->world.camera.rotation.y);
			mat4x4_rotate_Z(m, m, game->world.camera.rotation.z);
			//mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
			mat4x4_perspective(p, game->world.camera.fov, ratio, 0.1f, 200.f);
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
		void addPerson(Person* person, bool isPlacingPerson) {
			switch (person->type) {
			case PERSON_ARCHER:
				addMesh(archerMesh, person->pos, {.5f, .7f, .5f}, person->yRotation);
				break;
			case PERSON_CANNON:
				addMesh(cannonMesh, person->pos, {1.f, 1.f, 1.f}, person->yRotation);
				break;
			case PERSON_TURRET:
				addMesh(turretMesh, person->pos, {1.f, 1.f, 1.f}, person->yRotation);
				break;
			case PERSON_TANK:
				addMesh(tankMesh, person->pos, {1.f, 1.f, 1.f}, person->yRotation);
				break;
			case PERSON_GOLD_MINE:
				addMesh(goldMineMesh, person->pos, {1.f, 1.f, 1.f}, person->yRotation);
				break;
			case PERSON_BATTERY:
				addMesh(batteryMesh, person->pos, {1.f, 1.f, 1.f}, person->yRotation);
				break;
			case PERSON_SCIENTIST:
				addMesh(scientistMesh, person->pos, {1.f, 1.f, 1.f}, person->yRotation);
				break;
			case PERSON_ROBOT:
				addMesh(robotMesh, person->pos, {1.f, 1.f, 1.f}, person->yRotation);
				break;
			case PERSON_SCARECROW:
				addMesh(scarecrowMesh, person->pos, {1.f, person->stats.level < 4 ? 1.f : 2.f, 1.f}, person->yRotation);
				break;
			default:
				addCube(person->pos.x - person->stats.size.x / 2.f, person->pos.y, person->pos.z - person->stats.size.z / 2.f, person->stats.size.x, person->stats.size.y, person->stats.size.z, 0);
				break;
			}
			if (isPlacingPerson) {
				if (game->world.isPersonPlacable(&game->placingPerson)) {
					addPlane(person->pos.x - person->stats.range, person->pos.y + 0.1f, person->pos.z - person->stats.range, person->stats.range * 2.f, person->stats.range * 2.f, false, getMatID("range_green"));
				} else {
					addPlane(person->pos.x - person->stats.range, person->pos.y + 0.1f, person->pos.z - person->stats.range, person->stats.range * 2.f, person->stats.range * 2.f, false, getMatID("range_red"));

				}
			} else if (person->selected) {
				addPlane(person->pos.x - person->stats.range, person->pos.y + 0.1f, person->pos.z - person->stats.range, person->stats.range * 2.f, person->stats.range * 2.f, false, getMatID("range_blue"));
			}
		}

		void addCube(float x, float y, float z, float w, float h, float d, int matId) {
			unsigned int end = vertices.size();
			indiceses[matId].insert(indiceses[matId].end(), {
				2U+end, 7U+end, 6U+end,
				2U+end, 3U+end, 7U+end,

				0U+end, 5U+end, 4U+end,
				0U+end, 1U+end, 5U+end,

				0U+end, 6U+end, 4U+end,
				0U+end, 2U+end, 6U+end,

				1U+end, 7U+end, 3U+end,
				1U+end, 5U+end, 7U+end,

				0U+end, 3U+end, 2U+end,
				0U+end, 1U+end, 3U+end,

				4U+end, 6U+end, 7U+end,
				4U+end, 7U+end, 5U+end
			});
			vertices.insert(vertices.end(), {
				{x  , y  , z+d, 0.f, 0.f, 1.f},
				{x+w, y  , z+d, 1.f, 0.f, 1.f},
				{x  , y+h, z+d, 0.f, 1.f, 1.f},
				{x+w, y+h, z+d, 1.f, 1.f, 1.f},
				{x  , y  , z  , 0.f, 1.f, 1.f},
				{x+w, y  , z  , 1.f, 1.f, 1.f},
				{x  , y+h, z  , 0.f, 0.f, 1.f},
				{x+w, y+h, z  , 1.f, 0.f, 1.f}
			});
		}
		void addPlane(float x, float y, float z, float w, float d, bool worldUv, int matId) {
			unsigned int end = vertices.size();
			indiceses[matId].insert(indiceses[matId].end(), {
				0U+end, 1U+end, 2U+end,
				1U+end, 3U+end, 2U+end
			});
			vertices.insert(vertices.end(), {
				{x  , y , z+d, 0.f, 0.f, 1.f},
				{x+w, y , z+d, worldUv ? w : 1.f, 0.f, 1.f},
				{x  , y , z  , 0.f, worldUv ? d : 1.f, 1.f},
				{x+w, y , z  , worldUv ? w : 1.f, worldUv ? d : 1.f, 1.f}
			});
		}
		void addQuad(Vec3 p1, Vec3 p2, Vec3 p3, Vec3 p4, int matId) { // clockwise
			unsigned int end = vertices.size();
			indiceses[matId].insert(indiceses[matId].end(), {
				0U+end, 1U+end, 2U+end,
				0U+end, 2U+end, 3U+end
			});
			vertices.insert(vertices.end(), {
				{p1.x, p1.y, p1.z, 0.f, 0.f, 1.f},
				{p2.x, p2.y, p2.z, 1.f, 0.f, 1.f},
				{p3.x, p3.y, p3.z, 0.f, 1.f, 1.f},
				{p4.x, p4.y, p4.z, 1.f, 1.f, 1.f}
			});
		}
		void addPath(Vec3 p1, Vec3 p2, float width, float yOffset, int matId) {
			unsigned int end = vertices.size();
			float dist = distance3D(p1, p2);
			float xDiff = (p2.x - p1.x) / dist * width / 2.f;
			float zDiff = (p2.z - p1.z) / dist * width / 2.f;
			float endfake = 0.f;
			vertices.insert(vertices.end(), {
				{p1.x - zDiff - xDiff * endfake, p1.y + yOffset, p1.z + xDiff - zDiff * endfake, dist / width, 0.f, 1.f},
				{p1.x + zDiff - xDiff * endfake, p1.y + yOffset, p1.z - xDiff - zDiff * endfake, dist / width, 1.f, 1.f},
				{p2.x - zDiff + xDiff * endfake, p2.y + yOffset, p2.z + xDiff + zDiff * endfake, 0.f,		  0.f, 1.f},
				{p2.x + zDiff + xDiff * endfake, p2.y + yOffset, p2.z - xDiff + zDiff * endfake, 0.f,		  1.f, 1.f}
			});
			indiceses[matId].insert(indiceses[matId].end(), {
				0U+end, 2U+end, 1U+end,
				1U+end, 2U+end, 3U+end
			});
		}
		void addMesh(Mesh mesh, Vec3 position, Vec3 scale, float yRotation) {
			unsigned int end = vertices.size();
			for (int i = 0; i < (int)mesh.vertices.size(); i++) {
				mesh.vertices[i].x *= scale.x;
				mesh.vertices[i].y *= scale.y;
				mesh.vertices[i].z *= scale.z;
				if (yRotation != 0.f) {
					float x = mesh.vertices[i].x;
					float z = mesh.vertices[i].z;
					mesh.vertices[i].x = x * cos(yRotation) - z * sin(yRotation);
					mesh.vertices[i].z = z * cos(yRotation) + x * sin(yRotation);
				}
				mesh.vertices[i].x += position.x;
				mesh.vertices[i].y += position.y;
				mesh.vertices[i].z += position.z;
			}
			for (int i = 0; i < (int)mesh.indiceses.size(); i++) {
				for (int j = 0; j < (int)mesh.indiceses[i].size(); j++) {
					mesh.indiceses[i][j] += end;
				}
			}
			vertices.insert(vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
			for (int i = 0; i < (int)indiceses.size(); i++) {
				indiceses[i].insert(indiceses[i].end(), mesh.indiceses[i].begin(), mesh.indiceses[i].end());
			}
		}
		void addArc(Vec3 start, Vec3 end) {
			Vec3 pos = end;
			int i = 0;
			while (distance3D(pos, start) > 0.1f) {
				if (i >= 100) break;
				i++;
				Vec3 dir = vec3Add({randFloat() * 0.2f - 0.1f, 0.f, randFloat() * 0.2f - 0.1f}, vec3Subtract(start, pos).normalise(0.1f));//random + start - end
				addMesh(electricityMesh, pos, {0.2f, 1.f, 1.f}, atan2(dir.z, dir.x));
				pos = vec3Add(pos, dir);
			}
		}
		void addDisc(Vec3 pos, float r, int sides, int matID) {
			if (sides < 3) return;
			vector<Vertex> discVertices = {{pos.x, pos.y, pos.z, pos.x, pos.z}};
			vector<unsigned int> discIndices = {};
			unsigned int end = vertices.size();
			for (int i = 0; i < sides; i++) {
				float d = i / (float)sides * 3.141592653589f * 2.f;
				float x = sin(d) * r + pos.x;
				float z = cos(d) * r + pos.z;
				discVertices.push_back({x, pos.y, z, x, z});
			}
			for (int i = 1; i < (int)discVertices.size(); i++) {
				discIndices.push_back(0U + end);
				discIndices.push_back(i + end);
				discIndices.push_back((i % ((int)discVertices.size() - 1)) + 1U + end);
			}
			vertices.insert(vertices.end(), discVertices.begin(), discVertices.end());
			indiceses[matID].insert(indiceses[matID].end(), discIndices.begin(), discIndices.end());
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
				{x  , y+h, z, 0.f + uvOffset.x, 0.f + uvOffset.y, 0.f},
				{x+w, y+h, z, 1.f + uvOffset.x, 0.f + uvOffset.y, 0.f},
				{x  , y  , z, 0.f + uvOffset.x, 1.f + uvOffset.y, 0.f},
				{x+w, y  , z, 1.f + uvOffset.x, 1.f + uvOffset.y, 0.f}
			});
		}
		void addRect(float x, float y, float z, float w, float h, int matId) {
			unsigned int end = vertices.size();
			indiceses[matId].insert(indiceses[matId].end(), {
				0U+end, 2U+end, 1U+end,
				1U+end, 2U+end, 3U+end
			});
			vertices.insert(vertices.end(), {
				{x  , y+h, z, 0.f, 0.f, 0.f},
				{x+w, y+h, z, 1.f, 0.f, 0.f},
				{x  , y  , z, 0.f, 1.f, 0.f},
				{x+w, y  , z, 1.f, 1.f, 0.f}
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
	}
}