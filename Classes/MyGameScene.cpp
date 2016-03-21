#include "MyGameScene.h"

USING_NS_CC;
using namespace std;
string MyGame::map_file_name;


Scene* MyGame::createScene(string file_name) {
	auto scene = Scene::create();
	map_file_name = file_name;
	auto layer = MyGame::create();
	scene->addChild(layer);
	return scene;
}

bool MyGame::init() {
	if (!Layer::init()) {
		return false;
	}
	log("my game init");
	step = 0;
	game_map = GameMap::createWithTMXFile(map_file_name);
	CCASSERT(game_map, "gamemap create failed!");
	this->addChild(game_map);
	auto v = game_map->getProperty("speed");
	if (v.isNull()) {
		speed = 2;
		log("property speed has not define!");
	}
	else {
		speed = v.asInt();
	}
	auto snake_obj_group = game_map->getObjectGroup("snake_objs");
	CCASSERT(snake_obj_group, "snake_objs has not defined!");
	auto snake_objs = snake_obj_group->getObjects();
	for (auto _snake : snake_objs) {
		auto snake = _snake.asValueMap();
		auto sp = Snake::create(snake, game_map);
		if (snake["type"].asString() == "player") {
			if (player) {
				log("more than one player, will use the last one");
			}
			player = sp;
		}
		else {
			//AI->push_back(sp);
		}
	}
	CCASSERT(player, "player has not defined!");
	//game_map->addChild(player);
	auto label_score = Label::createWithSystemFont("score: 0", "Arial", SMALL_LABEL_FONT_SIZE);
	label_score->setAnchorPoint(Vec2(0.5, 1));
	label_score->setPosition(this->getContentSize().width / 2, 0);
	label_score->setName("score");
	this->addChild(label_score);

	auto label_goal = Label::createWithSystemFont(get_UTF8_string("goal"), "Arial", MID_LABEL_FONT_SIZE);
	label_goal->setAnchorPoint(Vec2(0, 1));
	label_goal->setPosition(origin + Vec2(0, visible_size.height));
	this->addChild(label_goal);

	scheduleUpdate();

	auto layer = LayerColor::create(Color4B::GRAY, game_map->getContentSize().width, game_map->getContentSize().height);
	if (layer->isIgnoreAnchorPointForPosition()) {
		layer->ignoreAnchorPointForPosition(false);
	}
	layer->setAnchorPoint(game_map->getAnchorPoint());
	layer->setPosition(game_map->getPosition());
	this->addChild(layer, -1);

	auto menu_back = MenuItemFont::create(get_UTF8_string("back"), [this](Ref *sender) {
		if (player->get_score() >= 3) {
			//GameMenu::position = GameMenu::nxt_pos;
			//GameMenu::direction = GameMenu::nxt_dir;
			user_info["game_menu_position_x"] = user_info["game_menu_nxt_pos_x"];
			user_info["game_menu_position_y"] = user_info["game_menu_nxt_pos_y"];
			user_info["game_menu_direction"] = user_info["game_menu_nxt_dir"];
			FileUtils::getInstance()->writeValueMapToFile(user_info, "user_info.xml");
		}
		//Director::getInstance()->popScene();
		auto next_scene = GameMenu::createScene();
		auto Transition_scene = TransitionCrossFade::create(SCENE_TURN_TRANSITION_TIME, next_scene);
		Director::getInstance()->replaceScene(Transition_scene);
	});
	menu_back->setAnchorPoint(Vec2::ZERO);
	auto menu = Menu::create(menu_back, NULL);
	menu->setAnchorPoint(Vec2::ZERO);
	menu->setPosition(Vec2::ZERO);
	this->addChild(menu);

	//add listener
	auto listener_touch = EventListenerTouchOneByOne::create();
	Vec2 *touch_begin = new Vec2();
	listener_touch->onTouchBegan = [touch_begin](Touch *t, Event *e) {
		auto position = t->getLocation();
		*touch_begin = position;
		return true;
	};
	auto set_dir = [](Vec2 v) {
		if (abs(v.x) > abs(v.y)) {
			if (v.x > 0) {
				return DIRECTION::RIGHT;
			}
			return DIRECTION::LEFT;
		}
		if (v.y > 0) {
			return DIRECTION::UP;
		}
		return DIRECTION::DOWN;
	};
	listener_touch->onTouchEnded = [this, set_dir](Touch *t, Event *e) {
		if (Rect(Vec2::ZERO, game_map->getContentSize()).containsPoint(game_map->convertToNodeSpace(t->getStartLocation()))) {
			auto control_mode = user_info["control_mode"].asInt();
			if (control_mode == 0 || control_mode == 2) {
				auto v = t->getLocation() - t->getStartLocation();
				if (v.isZero()) {
					v = t->getLocation() - (origin + visible_size / 2);
					DIRECTION dir = set_dir(v);
					player->turn(dir);
				}
			}
		}
		return true;
	};
	listener_touch->onTouchMoved = [this, set_dir, touch_begin](Touch *t, Event *e) {
		auto control_mode = user_info["control_mode"].asInt();
		if (control_mode == 0 || control_mode == 1) {
			auto pos = t->getLocation();
			auto start_pre = t->getPreviousLocation() - *touch_begin;
			auto pre_now = pos - t->getPreviousLocation();
			if (!start_pre.isZero() && abs(Vec2::angle(start_pre, pre_now)) > acos(-1.0f) / 4) {
				*touch_begin = pos;
			}
			if (fabs(pos.x - touch_begin->x) > touch_move_len || fabs(pos.y - touch_begin->y) > touch_move_len) {
				DIRECTION dir = set_dir(pos - *touch_begin);
				player->turn(dir);
				*touch_begin = pos;
			}
		}
	};
	auto listener_key = EventListenerKeyboard::create();
	listener_key->onKeyPressed = [this](EventKeyboard::KeyCode key, Event *e) {
		DIRECTION dir;
		switch (key) {
		case EventKeyboard::KeyCode::KEY_UP_ARROW:
		case EventKeyboard::KeyCode::KEY_W:
		case EventKeyboard::KeyCode::KEY_CAPITAL_W:
			dir = DIRECTION::UP;
			break;
		case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
		case EventKeyboard::KeyCode::KEY_S:
		case EventKeyboard::KeyCode::KEY_CAPITAL_S:
			dir = DIRECTION::DOWN;
			break;
		case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
		case EventKeyboard::KeyCode::KEY_A:
		case EventKeyboard::KeyCode::KEY_CAPITAL_A:
			dir = DIRECTION::LEFT;
			break;
		case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
		case EventKeyboard::KeyCode::KEY_D:
		case EventKeyboard::KeyCode::KEY_CAPITAL_D:
			dir = DIRECTION::RIGHT;
			break;
		default:
			return;
		}
		player->turn(dir);
	};
	Director::getInstance()->getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener_touch, this);
	Director::getInstance()->getEventDispatcher()->addEventListenerWithSceneGraphPriority(listener_key, this);
	return true;
}

void MyGame::update(float dt) {
	if (speed < 1) {
		speed = 1;
	}
	else if (speed > UNIT / 2) {
		speed = UNIT / 2;
	}
	//speed = 16;
	step += speed;
	if (step >= UNIT) {
		step -= UNIT;
		game_map->add_time_stamp(1);
		// test
		if (!player->get_is_died()) {
			//player->act();
			player->go_ahead();
			auto label = (Label*)this->getChildByName("score");
			label->setString("score: " + Value(player->get_score()).asString());
		}
		// test
		if (!player->get_is_died()) {
			player->check();
		}
	}
}