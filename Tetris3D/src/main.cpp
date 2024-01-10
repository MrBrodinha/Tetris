#include <iostream>
#include <fstream>
#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "GLSL.h"
#include "Program.h"
#include "MatrixStack.h"
#include "camera.h"

#include "WindowManager.h"
#include "Shape.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Board.h"
#include "Tetromino.h"

using namespace std;
using namespace glm;

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
bool leftButtonPressed = false;
float sensitivity = 1.0f; // Adjust this value to your liking

static int highscore = 0;

double get_last_elapsed_time();
bool isPastInterval(double interval);
void drawScore(shared_ptr<Program> tetrominoProg, shared_ptr<Shape> cube, int score, int highscore);
void drawNumber(shared_ptr<Program> tetrominoProg, shared_ptr<Shape> cube, int digit, int rowOffset, int colOffset);
void drawWordNext(shared_ptr<Program> tetrominoProg, shared_ptr<Shape> cube, int rowOffset, int colOffset);
void drawWordHold(shared_ptr<Program> tetrominoProg, shared_ptr<Shape> cube, int rowOffset, int colOffset);
void handleKeyboardInput(GLFWwindow *window);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);

class Application : public EventCallbacks
{

public:
    WindowManager *windowManager = nullptr;

    shared_ptr<Program> tetrominoProg, backgroundProg;

    shared_ptr<Shape> cube, sphere;

    GLuint BlockTexId, BackgroundTexId, NormalTexId, BlockTexId2;

    bool leftPress, rightPress, downPress, rotate, holdPress, spacePress = 0;

    void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
        {
            leftPress = true;
        }
        else if (key == GLFW_KEY_LEFT && action == GLFW_RELEASE)
        {
            leftPress = false;
        }
        else if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
        {
            rightPress = true;
        }
        else if (key == GLFW_KEY_RIGHT && action == GLFW_RELEASE)
        {
            rightPress = false;
        }
        else if (key == GLFW_KEY_UP && action == GLFW_PRESS)
        {
            rotate = true;
        }
        else if (key == GLFW_KEY_UP && action == GLFW_RELEASE)
        {
            rotate = false;
        }
        else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
        {
            spacePress = true;
        }
        else if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
        {
            spacePress = false;
        }
        else if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
        {
            downPress = true;
        }
        else if (key == GLFW_KEY_DOWN && action == GLFW_RELEASE)
        {
            downPress = false;
        }
        else if (key == GLFW_KEY_C && action == GLFW_PRESS)
        {
            holdPress = true;
        }
        else if (key == GLFW_KEY_C && action == GLFW_RELEASE)
        {
            holdPress = false;
        }
    }

    void mouseCallback(GLFWwindow *window, int button, int action, int mods) {}

    void resizeCallback(GLFWwindow *window, int in_width, int in_height)
    {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
    }

    void initGeom()
    {
        string resourceDirectory = "../res";
        cube = make_shared<Shape>();
        cube->loadMesh(resourceDirectory + "/cube.obj");
        cube->resize();
        cube->init();

        sphere = make_shared<Shape>();
        sphere->loadMesh(resourceDirectory + "/sphere.obj");
        sphere->resize();
        sphere->init();

        int width, height, channels;
        char filepath[1000];
        string str = resourceDirectory + "/blockTex.png";
        strcpy(filepath, str.c_str());
        unsigned char *data = stbi_load(filepath, &width, &height, &channels, 4);
        glGenTextures(1, &BlockTexId);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, BlockTexId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        GLuint Tex1Location = glGetUniformLocation(tetrominoProg->pid, "blockTex");
        glUseProgram(tetrominoProg->pid);
        glUniform1i(Tex1Location, 0);

        str = resourceDirectory + "/blockNormal.png";
        strcpy(filepath, str.c_str());
        data = stbi_load(filepath, &width, &height, &channels, 4);
        glGenTextures(1, &NormalTexId);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, NormalTexId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        GLuint Tex2Location = glGetUniformLocation(tetrominoProg->pid, "normalTex");
        glUseProgram(tetrominoProg->pid);
        glUniform1i(Tex2Location, 1);

        /*width, height, channels;
        filepath[1000];*/
        str = resourceDirectory + "/bitWallpaper.jpg";
        strcpy(filepath, str.c_str());
        data = stbi_load(filepath, &width, &height, &channels, 4);
        glGenTextures(1, &BackgroundTexId);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, BackgroundTexId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        GLuint Tex3Location = glGetUniformLocation(backgroundProg->pid, "backgroundTex");
        glUseProgram(backgroundProg->pid);
        glUniform1i(Tex3Location, 2);

        //add another block text that is blockTex2
        str = resourceDirectory + "/borda.png";
        strcpy(filepath, str.c_str());
        data = stbi_load(filepath, &width, &height, &channels, 4);
        glGenTextures(1, &BlockTexId2);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, BlockTexId2);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        GLuint Tex4Location = glGetUniformLocation(tetrominoProg->pid, "blockTex2");
        glUseProgram(tetrominoProg->pid);
        glUniform1i(Tex4Location, 3);
    }

    // General OGL initialization - set OGL state here
    void init(const std::string &resourceDirectory)
    {
        GLSL::checkVersion();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        tetrominoProg = std::make_shared<Program>();
        tetrominoProg->setVerbose(true);
        tetrominoProg->setShaderNames("../shaders/shader_vertex_block.glsl", "../shaders/shader_fragment_block.glsl");
        if (!tetrominoProg->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }

        backgroundProg = std::make_shared<Program>();
        backgroundProg->setVerbose(true);
        backgroundProg->setShaderNames("../shaders/shader_vertex_background.glsl", "../shaders/shader_fragment_background.glsl");
        if (!backgroundProg->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }

        tetrominoProg->addUniform("P");
        tetrominoProg->addUniform("V");
        tetrominoProg->addUniform("M");
        tetrominoProg->addUniform("camPos");
        tetrominoProg->addUniform("blockType");
        tetrominoProg->addAttribute("vertPos");
        tetrominoProg->addAttribute("vertNor");
        tetrominoProg->addAttribute("vertTex");
        tetrominoProg->addAttribute("normalTex");

        backgroundProg->addUniform("P");
        backgroundProg->addUniform("V");
        backgroundProg->addUniform("M");
        backgroundProg->addUniform("camPos");
        backgroundProg->addAttribute("vertPos");
        backgroundProg->addAttribute("vertNor");
        backgroundProg->addAttribute("vertTex");
        backgroundProg->addAttribute("backgroundTex");
    }

    void render(
        Board *board,
        Tetromino *currentTetromino,
        Tetromino *nextTetromino1, Tetromino *nextTetromino2, Tetromino *nextTetromino3, Tetromino *nextTetromino4,
        Tetromino *holdTetromino,
        Position *pos)
    {
        mat4 V = camera.GetViewMatrix();

        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        float aspect = width / (float)height;
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        mat4 P = ortho(-1 * aspect, 1 * aspect, -1.0f, 1.0f, -2.0f, 100.0f);
        if (width < height)
        {
            P = ortho(-1.0f, 1.0f, -1.0f / aspect, 1.0f / aspect, -2.0f, 100.0f);
        }
        P = perspective((float)(3.14159 / 2.2), (float)((float)width / (float)height), 0.1f, 1000.0f);

        static float angle = 3.14 / 2.0;
        mat4 skyTrans1 = translate(mat4(1), vec3(0, 0, -7.5f));
        mat4 skyTrans2 = translate(mat4(1), vec3(0, 0.8, 0));
        mat4 skyScale = scale(mat4(1), vec3(15, 15, 15));
        mat4 skyRotation = glm::rotate(mat4(1), angle, vec3(1.0f, 0.0f, 0.0f));
        mat4 skyM = skyTrans1 * skyTrans2 * skyRotation * skyScale;

        backgroundProg->bind();
        glDisable(GL_DEPTH_TEST);
        glUniformMatrix4fv(backgroundProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(backgroundProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
        glUniformMatrix4fv(backgroundProg->getUniform("M"), 1, GL_FALSE, &skyM[0][0]);
        glUniform3fv(backgroundProg->getUniform("camPos"), 1, &camera.Position[0]);
        sphere->draw(backgroundProg, false);
        backgroundProg->unbind();

        glEnable(GL_DEPTH_TEST);

        tetrominoProg->bind();
        glUniformMatrix4fv(tetrominoProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(tetrominoProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
        glUniform3fv(tetrominoProg->getUniform("camPos"), 1, &camera.Position[0]);
        tetrominoProg->unbind();

        mat4 tetrominoScaleMat = scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 0.1f));
        mat4 tetrominoTransMat, M;

        static Position ghostPos;
        static int score = 0;

        // tetromino moves down every second, need to check if elapsed time is >= 1 second
        if (isPastInterval(1))
        {
            pos->row++;
        }

        // check left, right and rotate
        if (leftPress)
        {
            if (isPastInterval(0.12))
            {
                pos->col--;
                if (board->isCollision(*currentTetromino, *pos))
                    pos->col++;
            }
        }
        else if (rightPress)
        {
            if (isPastInterval(0.12))
            {
                pos->col++;
                if (board->isCollision(*currentTetromino, *pos))
                    pos->col--;
            }
        }
        else if (downPress)
        {
            if (isPastInterval(0.12))
            {
                pos->row++;
            }
        }
        else if (rotate)
        {
            currentTetromino->rotation = (currentTetromino->rotation + 1) % 4;
            if (board->isCollision(*currentTetromino, *pos))
            {
                pos->col--;
            }
            rotate = false;
        }
        else if (spacePress)
        {
            while (!board->isCollision(*currentTetromino, *pos))
            {
                pos->row++;
            }
            spacePress = false;
        }
        else if (holdPress)
        {
            if (holdTetromino->type == -1)
            {
                holdTetromino->type = currentTetromino->type;
                holdTetromino->rotation = currentTetromino->rotation;

                currentTetromino->type = nextTetromino1->type;
                currentTetromino->rotation = nextTetromino1->rotation;

                nextTetromino1->type = nextTetromino2->type;
                nextTetromino1->rotation = nextTetromino2->rotation;

                nextTetromino2->type = nextTetromino3->type;
                nextTetromino2->rotation = nextTetromino3->rotation;

                nextTetromino3->type = nextTetromino4->type;
                nextTetromino3->rotation = nextTetromino4->rotation;

                nextTetromino4->type = rand() % 7;
                nextTetromino4->rotation = rand() % 4;
            }
            else
            {
                Tetromino temp;
                temp.type = currentTetromino->type;
                temp.rotation = currentTetromino->rotation;

                currentTetromino->type = holdTetromino->type;
                currentTetromino->rotation = holdTetromino->rotation;

                holdTetromino->type = temp.type;
                holdTetromino->rotation = temp.rotation;
            }
            while (board->isCollision(*currentTetromino, *pos))
            {
                if (pos->col > COLUMNS / 2)
                    pos->col--;
                else if (pos->col <= COLUMNS / 2)
                    pos->col++;
            }
            holdPress = false;
        }

        // check if collision, if so, store piece at pos->row-1;
        if (board->isCollision(*currentTetromino, *pos))
        {
            pos->row--;
            board->storeTetromino(*currentTetromino, *pos);
            currentTetromino->type = nextTetromino1->type;
            currentTetromino->rotation = nextTetromino1->rotation;
            nextTetromino1->type = nextTetromino2->type;
            nextTetromino1->rotation = nextTetromino2->rotation;
            nextTetromino2->type = nextTetromino3->type;
            nextTetromino2->rotation = nextTetromino3->rotation;
            nextTetromino3->type = nextTetromino4->type;
            nextTetromino3->rotation = nextTetromino4->rotation;
            nextTetromino4->type = rand() % 7;
            nextTetromino4->rotation = rand() % 4;
            pos->row = 0;
            pos->col = 5;
        }

        if (board->isGameOver())
        {
            board->clearBoard();
            holdTetromino->type = -1;
            holdTetromino->rotation = -1;
            highscore = score > highscore ? score : highscore;
            score = 0;
        }

        // draw current tetromino
        for (int row = 0; row < BLOCK_SIZE; row++)
        {
            for (int col = 0; col < BLOCK_SIZE; col++)
            {
                Position tetrominoPos;
                tetrominoPos.row = row;
                tetrominoPos.col = col;
                if (currentTetromino->getValue(tetrominoPos) != 0)
                {
                    tetrominoTransMat = translate(mat4(1), vec3((col + pos->col) / 5.0 - 1, -(row + pos->row) / 5.0 + 1.9, -6));
                    M = tetrominoTransMat * tetrominoScaleMat;
                    tetrominoProg->bind();
                    glUniformMatrix4fv(tetrominoProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
                    glUniform1i(tetrominoProg->getUniform("blockType"), currentTetromino->getValue(tetrominoPos));
                    cube->draw(tetrominoProg, false);
                    tetrominoProg->unbind();
                }
            }
        }

        // draw ghost tetromino
        ghostPos.row = pos->row;
        ghostPos.col = pos->col;

        while (!board->isCollision(*currentTetromino, ghostPos))
        {
            ghostPos.row++;
        }
        ghostPos.row--;

        for (int row = 0; row < BLOCK_SIZE; row++)
        {
            for (int col = 0; col < BLOCK_SIZE; col++)
            {
                Position tetrominoPos;
                tetrominoPos.row = row;
                tetrominoPos.col = col;
                if (currentTetromino->getValue(tetrominoPos) != 0)
                {
                    tetrominoTransMat = translate(mat4(1), vec3((col + ghostPos.col) / 5.0 - 1, -(row + ghostPos.row) / 5.0 + 1.9, -6));
                    M = tetrominoTransMat * tetrominoScaleMat;
                    tetrominoProg->bind();
                    glUniformMatrix4fv(tetrominoProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
                    glUniform1i(tetrominoProg->getUniform("blockType"), -1);
                    cube->draw(tetrominoProg, false);
                    tetrominoProg->unbind();
                }
            }
        }

        // draw next tetromino
        for (int row = 0; row < BLOCK_SIZE; row++)
        {
            for (int col = 0; col < BLOCK_SIZE; col++)
            {
                Position tetrominoPos;
                tetrominoPos.row = row;
                tetrominoPos.col = col;
                if (nextTetromino1->getValue(tetrominoPos) != 0)
                {
                    tetrominoTransMat = translate(mat4(1), vec3((col + 18) / 5.0 - 1, -(row + 7) / 5.0 + 1.9, -6));
                    M = tetrominoTransMat * tetrominoScaleMat;
                    tetrominoProg->bind();
                    glUniformMatrix4fv(tetrominoProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
                    glUniform1i(tetrominoProg->getUniform("blockType"), nextTetromino1->getValue(tetrominoPos));
                    cube->draw(tetrominoProg, false);
                    tetrominoProg->unbind();
                }

                if (nextTetromino2->getValue(tetrominoPos) != 0)
                {
                    tetrominoTransMat = translate(mat4(1), vec3((col + 18) / 5.0 - 1, -(row + 12) / 5.0 + 1.9, -6));
                    M = tetrominoTransMat * tetrominoScaleMat;
                    tetrominoProg->bind();
                    glUniformMatrix4fv(tetrominoProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
                    glUniform1i(tetrominoProg->getUniform("blockType"), nextTetromino2->getValue(tetrominoPos));
                    cube->draw(tetrominoProg, false);
                    tetrominoProg->unbind();
                }

                if (nextTetromino3->getValue(tetrominoPos) != 0)
                {
                    tetrominoTransMat = translate(mat4(1), vec3((col + 18) / 5.0 - 1, -(row + 17) / 5.0 + 1.9, -6));
                    M = tetrominoTransMat * tetrominoScaleMat;
                    tetrominoProg->bind();
                    glUniformMatrix4fv(tetrominoProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
                    glUniform1i(tetrominoProg->getUniform("blockType"), nextTetromino3->getValue(tetrominoPos));
                    cube->draw(tetrominoProg, false);
                    tetrominoProg->unbind();
                }

                if (nextTetromino4->getValue(tetrominoPos) != 0)
                {
                    tetrominoTransMat = translate(mat4(1), vec3((col + 18) / 5.0 - 1, -(row + 23) / 5.0 + 1.9, -6));
                    M = tetrominoTransMat * tetrominoScaleMat;
                    tetrominoProg->bind();
                    glUniformMatrix4fv(tetrominoProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
                    glUniform1i(tetrominoProg->getUniform("blockType"), nextTetromino4->getValue(tetrominoPos));
                    cube->draw(tetrominoProg, false);
                    tetrominoProg->unbind();
                }
            }
        }

        // draw hold tetromino
        if (holdTetromino->type != -1)
        {
            for (int row = 0; row < BLOCK_SIZE; row++)
            {
                for (int col = 0; col < BLOCK_SIZE; col++)
                {
                    Position tetrominoPos;
                    tetrominoPos.row = row;
                    tetrominoPos.col = col;
                    if (holdTetromino->getValue(tetrominoPos) != 0)
                    {
                        tetrominoTransMat = translate(mat4(1), vec3((col - 10) / 5.0 - 1, -(row + 7) / 5.0 + 1.9, -6));
                        M = tetrominoTransMat * tetrominoScaleMat;
                        tetrominoProg->bind();
                        glUniformMatrix4fv(tetrominoProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
                        glUniform1i(tetrominoProg->getUniform("blockType"), holdTetromino->getValue(tetrominoPos));
                        cube->draw(tetrominoProg, false);
                        tetrominoProg->unbind();
                    }
                }
            }
        }

        drawScore(tetrominoProg, cube, score, highscore);
        drawWordNext(tetrominoProg, cube, -1, 13);
        drawWordHold(tetrominoProg, cube, -1, -22);

        // draw board
        for (int row = 0; row < 20; row++)
        {
            for (int col = 0; col < 10; col++)
            {
                if (board->board[row][col] != 0)
                {
                    tetrominoTransMat = translate(mat4(1), vec3(col / 5.0 - 1, -row / 5.0 + 1.9, -6));
                    M = tetrominoTransMat * tetrominoScaleMat;
                    tetrominoProg->bind();
                    glUniformMatrix4fv(tetrominoProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
                    glUniform1i(tetrominoProg->getUniform("blockType"), board->board[row][col]);
                    cube->draw(tetrominoProg, false);
                    tetrominoProg->unbind();
                }
            }
        }
        for (int row = -1; row <= ROWS; row++)
        {
            for (int col = -1; col <= COLUMNS; col++)
            {
                if (row == -1 || row == ROWS || col == -1 || col == COLUMNS)
                {
                    tetrominoTransMat = translate(mat4(1), vec3(col / 5.0 - 1, -row / 5.0 + 1.9, -6));
                    M = tetrominoTransMat * tetrominoScaleMat;
                    tetrominoProg->bind();
                    glUniformMatrix4fv(tetrominoProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
                    glUniform1i(tetrominoProg->getUniform("blockType"), -2);
                    cube->draw(tetrominoProg, false);
                    tetrominoProg->unbind();
                }
            }
        }
        score += board->deleteFullLines();
    }
};

int main(int argc, char **argv)
{
    std::string resourceDir = "../resources";
    if (argc >= 2)
    {
        resourceDir = argv[1];
    }

    Application *application = new Application();

    WindowManager *windowManager = new WindowManager();
    windowManager->init(SCR_WIDTH, SCR_HEIGHT);
    windowManager->setEventCallbacks(application);
    application->windowManager = windowManager;

    GLFWwindow *window = windowManager->getHandle();
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    application->init(resourceDir);
    application->initGeom();

    srand(time(NULL));
    Board *board = new Board();
    Position currentPos;
    Tetromino currentTetromino, holdTetromino, nextTetromino1, nextTetromino2, nextTetromino3, nextTetromino4;

    currentPos.row = 0;
    currentPos.col = 5;

    currentTetromino.type = rand() % 7;
    currentTetromino.rotation = rand() % 4;

    nextTetromino1.type = rand() % 7;
    nextTetromino1.rotation = rand() % 4;

    nextTetromino2.type = rand() % 7;
    nextTetromino2.rotation = rand() % 4;

    nextTetromino3.type = rand() % 7;
    nextTetromino3.rotation = rand() % 4;
    
    nextTetromino4.type = rand() % 7;
    nextTetromino4.rotation = rand() % 4;

    holdTetromino.type = -1;
    holdTetromino.rotation = -1;

    while (!glfwWindowShouldClose(window))
    {

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        application->render(board, &currentTetromino, &nextTetromino1, &nextTetromino2, &nextTetromino3, &nextTetromino4, &holdTetromino, &currentPos);
        handleKeyboardInput(window);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

double get_last_elapsed_time()
{
    static double lasttime = glfwGetTime();
    double actualtime = glfwGetTime();
    double difference = actualtime - lasttime;
    lasttime = actualtime;
    return difference;
}

bool isPastInterval(double interval)
{
    static double previousTime = glfwGetTime();
    double currentTime = glfwGetTime();
    if (currentTime - previousTime >= interval)
    {
        previousTime = currentTime;
        return true;
    }
    return false;
}

void drawScore(shared_ptr<Program> tetrominoProg, shared_ptr<Shape> cube, int score, int highscore)
{
    int counter = 0;
    int digit = 0;
    int value = score;
    vector<int> digits;

    if (value == 0)
        drawNumber(tetrominoProg, cube, 0, 16, 13);

    while (value > 0)
    {
        digit = value % 10;
        value /= 10;
        digits.push_back(digit);
    }

    for (std::vector<int>::size_type index = 0; index < digits.size(); index++)
    {
        drawNumber(tetrominoProg, cube, digits.at(index), 16, 13 + (5 * counter));
        counter++;
    }

    counter = 0;
    digit = 0;
    value = highscore;
    vector<int> digits2;

    if (value == 0)
        drawNumber(tetrominoProg, cube, 0, 24, -7);

    while (value > 0)
    {
        digit = value % 10;
        value /= 10;
        digits2.push_back(digit);
    }

    for (std::vector<int>::size_type index = 0; index < digits2.size(); index++)
    {
        drawNumber(tetrominoProg, cube, digits2.at(index), 24, -7 + (5 * counter));
        counter++;
    }
}

void drawNumber(shared_ptr<Program> tetrominoProg, shared_ptr<Shape> cube, int digit, int rowOffset, int colOffset)
{
    int numbers[10][5][4] = {
        {// 0
         {1, 1, 1, 1},
         {1, 0, 0, 1},
         {1, 0, 0, 1},
         {1, 0, 0, 1},
         {1, 1, 1, 1}},
        {// 1
         {0, 1, 1, 0},
         {0, 0, 1, 0},
         {0, 0, 1, 0},
         {0, 0, 1, 0},
         {0, 1, 1, 1}},
        {// 2
         {1, 1, 1, 1},
         {0, 0, 0, 1},
         {1, 1, 1, 1},
         {1, 0, 0, 0},
         {1, 1, 1, 1}},
        {// 3
         {1, 1, 1, 1},
         {0, 0, 0, 1},
         {1, 1, 1, 1},
         {0, 0, 0, 1},
         {1, 1, 1, 1}},
        {// 4
         {1, 0, 0, 1},
         {1, 0, 0, 1},
         {1, 1, 1, 1},
         {0, 0, 0, 1},
         {0, 0, 0, 1}},
        {// 5
         {1, 1, 1, 1},
         {1, 0, 0, 0},
         {1, 1, 1, 1},
         {0, 0, 0, 1},
         {1, 1, 1, 1}},
        {// 6
         {1, 1, 1, 1},
         {1, 0, 0, 0},
         {1, 1, 1, 1},
         {1, 0, 0, 1},
         {1, 1, 1, 1}},
        {// 7
         {1, 1, 1, 1},
         {0, 0, 0, 1},
         {0, 0, 0, 1},
         {0, 0, 0, 1},
         {0, 0, 0, 1}},
        {// 8
         {1, 1, 1, 1},
         {1, 0, 0, 1},
         {1, 1, 1, 1},
         {1, 0, 0, 1},
         {1, 1, 1, 1}},
        {// 9
         {1, 1, 1, 1},
         {1, 0, 0, 1},
         {1, 1, 1, 1},
         {0, 0, 0, 1},
         {0, 0, 0, 1}}};
    mat4 blockScaleMat = scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 0.1f));
    mat4 blockTransMat;

    for (int row = 0; row < 5; row++)
    {
        for (int col = 0; col < 4; col++)
        {
            if (numbers[digit][row][col] != 0)
            {
                blockTransMat = translate(mat4(1), vec3(-(-col + colOffset) / 5.0 - 1, -(row + rowOffset) / 5.0 + 1.9, -6));
                mat4 M = blockTransMat * blockScaleMat;
                tetrominoProg->bind();
                glUniformMatrix4fv(tetrominoProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
                glUniform1i(tetrominoProg->getUniform("blockType"), -3);
                cube->draw(tetrominoProg, false);
                tetrominoProg->unbind();
            }
        }
    }
}

void drawWordNext(shared_ptr<Program> tetrominoProg, shared_ptr<Shape> cube, int rowOffset, int colOffset)
{
    int word[4][5][4] = {
        {// N
         {1, 0, 0, 1},
         {1, 1, 0, 1},
         {1, 1, 1, 1},
         {1, 0, 1, 1},
         {1, 0, 0, 1}},
        {// E
         {1, 1, 1, 1},
         {1, 1, 0, 0},
         {1, 1, 1, 0},
         {1, 1, 0, 0},
         {1, 1, 1, 1}},
        {// X
         {1, 0, 0, 1},
         {1, 1, 1, 1},
         {0, 1, 1, 0},
         {1, 0, 0, 1},
         {1, 0, 0, 1}},
        {// T
         {1, 1, 1, 1},
         {1, 1, 1, 1},
         {0, 1, 1, 0},
         {0, 1, 1, 0},
         {0, 1, 1, 0}}};

    mat4 blockScaleMat = scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 0.1f));
    mat4 blockTransMat;

    for (int letter = 0; letter < 4; letter++)
    {
        for (int row = 0; row < 5; row++)
        {
            for (int col = 0; col < 4; col++)
            {
                if (word[letter][row][col] != 0)
                {
                    blockTransMat = translate(mat4(1), vec3((col + (colOffset + (letter * 5))) / 5.0 - 1, -(row + rowOffset) / 5.0 + 1.9, -6));
                    mat4 M = blockTransMat * blockScaleMat;
                    tetrominoProg->bind();
                    glUniformMatrix4fv(tetrominoProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
                    glUniform1i(tetrominoProg->getUniform("blockType"), -3);
                    cube->draw(tetrominoProg, false);
                    tetrominoProg->unbind();
                }
            }
        }
    }
}

void drawWordHold(shared_ptr<Program> tetrominoProg, shared_ptr<Shape> cube, int rowOffset, int colOffset)
{
    int word[4][5][4] = {
        {// H
         {1, 0, 0, 1},
         {1, 1, 1, 1},
         {1, 1, 1, 1},
         {1, 0, 0, 1},
         {1, 0, 0, 1}},
        {// O
         {1, 1, 1, 1},
         {1, 0, 0, 1},
         {1, 0, 0, 1},
         {1, 0, 0, 1},
         {1, 1, 1, 1}},
        {// L
         {1, 0, 0, 0},
         {1, 0, 0, 0},
         {1, 0, 0, 0},
         {1, 0, 0, 0},
         {1, 1, 1, 1}},
        {// D
         {1, 1, 1, 0},
         {1, 0, 0, 1},
         {1, 0, 0, 1},
         {1, 0, 0, 1},
         {1, 1, 1, 0}}};

    mat4 blockScaleMat = scale(glm::mat4(1.0f), glm::vec3(0.1f, 0.1f, 0.1f));
    mat4 blockTransMat;

    for (int letter = 0; letter < 4; letter++)
    {
        for (int row = 0; row < 5; row++)
        {
            for (int col = 0; col < 4; col++)
            {
                if (word[letter][row][col] != 0)
                {
                    blockTransMat = translate(mat4(1), vec3((col + (colOffset + (letter * 5))) / 5.0 - 1, -(row + rowOffset) / 5.0 + 1.9, -6));
                    mat4 M = blockTransMat * blockScaleMat;
                    tetrominoProg->bind();
                    glUniformMatrix4fv(tetrominoProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
                    glUniform1i(tetrominoProg->getUniform("blockType"), -3);
                    cube->draw(tetrominoProg, false);
                    tetrominoProg->unbind();
                }
            }
        }
    }
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void handleKeyboardInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    // Add more controls as needed
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        if (firstMouse)
        {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

        xoffset *= sensitivity;
        yoffset *= sensitivity;

        lastX = xpos;
        lastY = ypos;

        camera.ProcessMouseMovement(xoffset, yoffset);
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}