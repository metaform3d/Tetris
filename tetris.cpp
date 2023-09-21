#include <SFML/Graphics.hpp>
#include "Support.hpp"

class Tetris {
public:
	const static int GRID_WIDTH = 10;
	const static int GRID_HEIGHT = 20;

	class ValueGrid {
	public:
		virtual int value(Square pos) const = 0;
	};

	class Boundary : public ValueGrid {
	public:
		int value(Square pos) const override {
			if (pos.x < 0 || pos.x >= GRID_WIDTH || pos.y >= GRID_HEIGHT) {
				return -1;
			}
			return 0;
		}
	};

	class Storage : public ValueGrid {
	public:
		struct LineRange {
			int start = 0;
			int count = 0;
		};
		Square::Field<int> map;
		int linesCleared = 0;
		float hangTime = 1.f;

		Storage() : map(GRID_WIDTH, GRID_HEIGHT, 0) { }

		int value(Square pos) const override {
			if (map.grid.contains(pos)) {
				return map[pos];
			}
			return 0;
		}

		int lineUsage(int y) const {
			int count = 0;
			for (int x = 0; x < GRID_WIDTH; x++) {
				if (value(Square(x, y)) != 0) {
					count++;
				}
			}
			return count;
		}

		LineRange fullLines() const {
			LineRange range;
			for (int y = 0; y < GRID_HEIGHT; y++) {
				if (lineUsage(y) != GRID_WIDTH) {
					if (range.count != 0) {
						break;
					}
					continue;
				}
				if (range.count == 0) {
					range.start = y;
					range.count = 1;
				} else {
					range.count++;
				}
			}
			return range;
		}

		int clearLines(LineRange const& range) {
			if (range.count <= 0) {
				return 0;
			}
			for (int y = range.start + range.count - 1; y >= 0; y--) {
				for (int x = 0; x < GRID_WIDTH; x++) {
					map[Square(x, y)] = value(Square(x, y - range.count));
				}
			}
			linesCleared += range.count;
			hangTime = hangTime * 0.99f;
			return range.count;
		}
	};

	class Glob : public ValueGrid {
	public:
		Square deltas[4] = { {0,0}, {0,0}, {0,0}, {0,0} };
		Square position = { -10,-10 };
		sf::Vector2f center;
		int color = -2;

		void shape(Square const d[4], float centerX = 0, float centerY = 0) {
			for (int i = 0; i < 4; i++) {
				deltas[i] = d[i];
			}
			center = { centerX, centerY };
		}

		void setup(int color) {
			position.x = GRID_WIDTH / 2;
			position.y = -1;
			if (color == 1) {
				Square d[4] = { { -2, 0 }, { -1, 0 }, { 0, 0 }, { 1, 0 } };
				shape(d, -0.5f, 0.5f);
			} else if (color == 2) {
				Square d[4] = { { -1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 } };
				shape(d);
			} else if (color == 3) {
				Square d[4] = { { 1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 } };
				shape(d);
			} else if (color == 4) {
				Square d[4] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } };
				shape(d, 0.5f, 0.5f);
				position.x -= 1;
			} else if (color == 5) {
				Square d[4] = { { 0, 0 }, { 1, 0 }, { -1, 1 }, { 0, 1 } };
				shape(d);
			} else if (color == 6) {
				Square d[4] = { { 0, -1 }, { -1, 0 }, { 0, 0 }, { 1, 0 } };
				shape(d);
			} else if (color == 7) {
				Square d[4] = { { -1, 0 }, { 0, 0 }, { 0, 1 }, { 1, 1 } };
				shape(d);
			} else {
				assert(false);
			}
			this->color = color;
		}

		int value(Square pos) const override {
			for (int i = 0; i < 4; i++) {
				if (deltas[i] == pos - position) {
					return color;
				}
			}
			return 0;
		}

		bool collides(ValueGrid const& background, Square offset) const {
			for (int i = 0; i < 4; i++) {
				Square below = deltas[i] + position + offset;
				if (background.value(below) != 0) {
					return true;
				}
			}
			return false;
		}

		void rotate(bool clockwise) {
			for (int i = 0; i < 4; i++) {
				auto q = rotate90(deltas[i].asFloat() - center, !clockwise);
				deltas[i] = Square(q + center);
			}
		}

		void slamDown(ValueGrid const& background) {
			while (!collides(background, Square(0, 1))) {
				position.y += 1;
			}
		}

		void stamp(Storage& storage) const {
			for (int i = 0; i < 4; i++) {
				Square dest = deltas[i] + position;
				if (storage.map.grid.contains(dest)) {
					storage.map[dest] = color;
				}
			}
		}
	};

	class Composition : public ValueGrid {
	public:
		std::vector<ValueGrid const *> stack;

		void append(ValueGrid const& ref) {
			stack.push_back(&ref);
		}

		int value(Square pos) const override {
			for (auto grid : stack) {
				int val = grid->value(pos);
				if (val != 0) {
					return val;
				}
			}
			return 0;
		}
	};

	class Logic {
	public:
		Square::Grid grid;
		Boundary boundary;
		Storage storage;
		Glob mino, ghost;
		Composition background;
		Composition colorBlocks;
		RandomF rng;

		Logic() : grid(GRID_WIDTH, GRID_HEIGHT)
		{
			background.append(storage);
			background.append(boundary);
			colorBlocks.append(mino);
			colorBlocks.append(background);
			newMino();
		}

		void updateGhost() {
			ghost = mino;
			ghost.color = -10;
			ghost.slamDown(background);
		}

		void newMino() {
			mino.setup(rng.intrange(1, 7));
			updateGhost();
		}

		void moveDown() {
			if (mino.collides(background, Square(0, 1))) {
				mino.stamp(storage);
				newMino();
				storage.clearLines(storage.fullLines());
				updateGhost();
			} else {
				mino.position.y += 1;
			}
		}

		void moveSide(int delta) {
			if (!mino.collides(background, Square(delta, 0))) {
				mino.position.x += delta;
			}
			updateGhost();
		}

		void rotate(bool cw) {
			mino.rotate(cw);
			if (mino.collides(storage, Square(0, 0))) {
				mino.rotate(!cw);
				return;
			}
			int startX = mino.position.x;
			int sign = startX < GRID_WIDTH / 2 ? 1 : -1;
			while (mino.collides(boundary, Square(0, 0))) {
				mino.position.x += sign;
			}
			updateGhost();
		}

		void slamDown() {
			mino.slamDown(background);
			moveDown();
		}

		bool gameOver() const {
			return storage.lineUsage(0) != 0;
		}
	};

	class Game : public GameInterface {
	public:
		const int BLOCK_PIXELS = 32;

		std::unique_ptr<Logic> tetris{ new Logic };
		float dropTimer = 0;

		std::string gi_windowName() override {
			return "Tetris";
		}

		sf::Vector2u gi_windowSize() override {
			return sf::Vector2u(tetris->grid.width * BLOCK_PIXELS,
								tetris->grid.height * BLOCK_PIXELS);
		}

		sf::Color gi_clearColor() override {
			return sf::Color(128, 128, 128);
		}

		class BlockPass {
		public:
			sf::RectangleShape block;
			virtual void draw(sf::RenderWindow& window, int value) {
				window.draw(block);
			}
		};

		class ColorPass : public BlockPass {
		public:
			const sf::Uint8 colors[7 * 3] = {
				105, 225, 240,  70, 115, 195,  225, 135, 40,  240, 240, 90,
				95, 240, 95,  175, 60, 240,  240, 30, 30
			};

			void draw(sf::RenderWindow& window, int value) override {
				int b = value * 3 - 3;
				block.setFillColor(sf::Color(colors[b], colors[b + 1], colors[b + 2]));
				window.draw(block);
			}
		};

		void drawPass(sf::RenderWindow& window, ValueGrid const& vals, BlockPass &pass) {
			auto scan = tetris->grid.allSquares();
			for (int i = 0; i < scan.count(); i++) {
				auto pos = scan.next();
				int value = vals.value(pos);
				if (value != 0) {
					pass.block.setPosition((pos * BLOCK_PIXELS).asFloat());
					pass.draw(window, value);
				}
			}
		}
			
		void gi_redraw(sf::RenderWindow& window) override {
			float size = float(BLOCK_PIXELS);
			BlockPass common;
			common.block.setSize({ size, size });
			common.block.setFillColor(sf::Color(145, 145, 185));
			drawPass(window, tetris->ghost, common);

			ColorPass colorPass;
			colorPass.block.setSize({ size - 2, size - 2 });
			colorPass.block.setOrigin(-1.f, -1.f);
			drawPass(window, tetris->colorBlocks, colorPass);

			common.block = colorPass.block;
			common.block.setFillColor(sf::Color::Transparent);
			common.block.setOutlineColor(sf::Color::White);
			common.block.setOutlineThickness(2.f);
			drawPass(window, tetris->colorBlocks, common);
		}

		void checkGameOver() {
			if (tetris->gameOver()) {
				printf("game over, score: %d\n" + tetris->storage.linesCleared);
				tetris = std::make_unique<Logic>();
			}
		}

		void dropDown() {
			tetris->moveDown();
			checkGameOver();
			dropTimer = tetris->storage.hangTime;
		}

		void gi_keyOnce(sf::Keyboard::Scancode code) override {
			if (code == sf::Keyboard::Scan::Left) {
				tetris->moveSide(-1);
			} else if (code == sf::Keyboard::Scan::Right) {
				tetris->moveSide(1);
			} else if (code == sf::Keyboard::Scan::X) {
				tetris->rotate(true);
			} else if (code == sf::Keyboard::Scan::Z) {
				tetris->rotate(false);
			} else if (code == sf::Keyboard::Scan::C) {
				tetris->slamDown();
			} else if (code == sf::Keyboard::Scan::Down) {
				dropDown();
			}
		}

		void gi_update(float dt) override {
			dropTimer -= dt;
			if (dropTimer < 0) {
				dropDown();
			}
		}
	};
};

bool randomNeedSeeding = true;

int main() {
	Tetris::Game application;
	application.run();
	return 0;
}