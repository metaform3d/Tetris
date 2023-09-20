#include <SFML/Graphics.hpp>
#include "GameInterface.hpp"
#include "Square.hpp"
#include "Neural.hpp"

#include "AntWorld.hpp"

extern void runTests();
extern void runTetris(void);
extern NeuralNetwork testXOREvolution();

sf::Image genImage(NeuralNetwork const& nn) {
    sf::Image image;
    image.create(200, 200);
    Square::Grid grid(200, 200);
    for (unsigned int x = 0; x < 200; x++) {
        for (unsigned int y = 0; y < 200; y++) {
            auto xy = grid.vecInclude(Square(x, y));

            float v = nn.evaluate(MatrixF::column({ xy.x, xy.y }))[0];
            image.setPixel(x, y, ColorF::grey(v));
        }
    }
    return image;
}

class NothingGame: public GameInterface
{
public:
    sf::Texture texture;
    float z = 0.0;

    NothingGame() {
        auto nn = testXOREvolution();
        sf::Image image = genImage(nn);
        auto size = image.getSize();
        if (!texture.create(size.x, size.y)) {
            assert(false);
        }
        texture.update(image);
    }

    ~NothingGame() = default;

    void gi_redraw(sf::RenderWindow& window) {
        sf::CircleShape shape(50.f);
        shape.setFillColor(sf::Color(150, 50, 250));
        shape.setPosition(100 + z, 200 + z);
        shape.setOutlineThickness(10.f);
        shape.setOutlineColor(sf::Color(250, 150, 100));

        window.draw(shape);

        sf::RectangleShape rect(sf::Vector2f(2, 2));
        rect.setFillColor(sf::Color(199, 200, 201));
        auto ran = RandomF();
        for (int i = 0; i < 500; i++) {
            sf::Vector2f xy(ran.normal(100), ran.normal(100));
            rect.setPosition(xy + sf::Vector2f(300, 300));
            window.draw(rect);
        }

        sf::Sprite sprite;
        sprite.setTexture(texture);
        sprite.setOrigin(z, z);
        sprite.setPosition(500, 200);
        sprite.setScale(1 + z / 60, 1 + z / 60);
        window.draw(sprite);
    }

    void gi_update(float dt) {
        z += 0.1f;
    }
};

int main()
{
    runTetris();

    //runTests();
    
    //auto g = NothingGame();
    //g.run();;
    
    //AntWorld::Params p;
    //auto g = TestAntApp<RandomAntBrain>(p);
    //g.run();

    return 0;
}
