#pragma once

#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <cassert>

template <typename Element>
std::vector<Element> repeating(Element const& val, int count)
{
    std::vector<Element> result;
    result.reserve(count);
    for (int i = 0; i < count; i++) {
        result.push_back(val);
    }
    return result;
}

template <typename Scalar>
sf::Vector2<Scalar> rotate90(sf::Vector2<Scalar> p, bool clockwise) {
	if (clockwise) {
		return { p.y, -p.x };
	} else {
		return { -p.y, p.x };
	}
}

struct Square
{
	int x;
	int y;

	Square(int xx, int yy) : x(xx), y(yy) { }

	Square(sf::Vector2f vec) : x(int(round(vec.x))), y(int(round(vec.y))) { }

	Square operator+ (const Square& rhs) const {
		return Square(x + rhs.x, y + rhs.y);
	}

	Square operator- (const Square& rhs) const {
		return Square(x - rhs.x, y - rhs.y);
	}

	Square operator* (int value) const {
		return Square(x * value, y * value);
	}

	bool operator< (const Square& other) const {
		if (y != other.y) {
			return y < other.y;
		}
		return x < other.x;
	}

	bool operator== (const Square& other) const {
		return x == other.x && y == other.y;
	}

	sf::Vector2f asFloat() const {
		return sf::Vector2f(float(x), float(y));
	}

	struct Grid
	{
		int width;
		int height;

		Grid(int w, int h) : width(w), height(h) { }

		Grid(Grid const& g) : width(g.width), height(g.height) { }

		int size() const {
			return width * height;
		}

		bool contains(Square const& spot) const {
			return spot.x >= 0 && spot.x < width && spot.y >= 0 && spot.y < height;
		}

		int index(Square const& spot) const {
			return spot.y * width + spot.x;
		}

		Square square(int index) const {
			return Square(index % width, index / width);
		}

		struct Scan
		{
			int w, h;
			int x = 0;
			int y = 0;

			Scan(int w, int h) : w(w), h(h) { }

			int count() const {
				return w * h;
			}

			Square next() {
				Square current = Square(x, y);
				x++;
				if (x == w) {
					x = 0;
					y++;
					if (y == h) {
						y = 0;
					}
				}
				return current;
			}
		};

		Scan allSquares() const {
			return Scan(width, height);
		}
	};

	template <typename T>
	struct Field
	{
		Grid grid;
		std::vector<T> values;

		Field(int w, int h, T emptyValue) : grid(w, h) {
			values = repeating(emptyValue, grid.size());
		}

		Field(Grid const& g, T emptyValue) : grid(g) {
			values = repeating(emptyValue, grid.size());
		}

		T& operator[](Square spot) {
			assert(grid.contains(spot));
			return values.data()[grid.index(spot)];
		}

		T operator[](Square spot) const {
			assert(grid.contains(spot));
			return values.data()[grid.index(spot)];
		}
	};
};

class GameInterface 
{
public:
	virtual ~GameInterface() = default;

    virtual sf::Vector2u gi_windowSize() {
        return sf::Vector2u(800, 600);
    }

    virtual std::string gi_windowName() {
        return "Abstract";
    }

    virtual sf::Color gi_clearColor() { 
        return sf::Color::Black;
    }

    virtual void gi_redraw(sf::RenderWindow& window) = 0;
    virtual void gi_update(float dt) { }
    virtual void gi_event(sf::Event& event) { }
    virtual void gi_keyOnce(sf::Keyboard::Scancode key) { }

    void run()
    {
        sf::Vector2u winSize = gi_windowSize();
        sf::RenderWindow window(sf::VideoMode(winSize.x, winSize.y), gi_windowName().c_str());
        window.setVerticalSyncEnabled(true);

        sf::Clock clock;
        sf::Keyboard::Scancode currentKeyDown = sf::Keyboard::Scancode::Unknown;

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                } else {
                    gi_event(event);

                    if (event.type == sf::Event::KeyPressed) {
                        auto code = event.key.scancode;
                        if (code != currentKeyDown) {
                            currentKeyDown = code;
                            gi_keyOnce(code);
                        }
                    } else if (event.type == sf::Event::KeyReleased) {
                        currentKeyDown = sf::Keyboard::Scancode::Unknown;
                    }
                }
            }

            sf::Time elapsed = clock.restart();
            gi_update(elapsed.asSeconds());

            window.clear(gi_clearColor());
            gi_redraw(window);
            window.display();
        }
    }
};

extern bool randomNeedSeeding;

template <typename Scalar>
struct Random {
	Random() {
		if (randomNeedSeeding) {
			srand((unsigned int)time(nullptr));
			randomNeedSeeding = false;
		}
	}

	Scalar unit() const {
		return Scalar(rand() / (RAND_MAX + 1.0));
	}

	int intrange(int lo, int hi) const {
		return int(unit() * (hi - lo + 1) + lo);
	}
};

using RandomF = Random<float>;
