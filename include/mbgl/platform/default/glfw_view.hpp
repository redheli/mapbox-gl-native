#ifndef MBGL_COMMON_GLFW_VIEW
#define MBGL_COMMON_GLFW_VIEW

#include <mbgl/mbgl.hpp>
#include <mbgl/util/run_loop.hpp>

#ifdef MBGL_USE_GLES2
#define GLFW_INCLUDE_ES2
#endif
#include <GLFW/glfw3.h>

#include <atomic>

class GLFWView : public mbgl::View {
public:
    GLFWView(bool fullscreen = false, bool benchmark = false);
    ~GLFWView();

    float getPixelRatio() const override;
    std::array<uint16_t, 2> getSize() const override;
    std::array<uint16_t, 2> getFramebufferSize() const override;

    void initialize(mbgl::Map *map) override;
    void activate() override;
    void deactivate() override;
    void notify() override;
    void invalidate() override;
    void beforeRender() override;
    void afterRender() override;

    static void onKey(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void onScroll(GLFWwindow *window, double xoffset, double yoffset);
    static void onWindowResize(GLFWwindow *window, int width, int height);
    static void onFramebufferResize(GLFWwindow *window, int width, int height);
    static void onMouseClick(GLFWwindow *window, int button, int action, int modifiers);
    static void onMouseMove(GLFWwindow *window, double x, double y);

    // Callback called when the user presses the key mapped to style change.
    // The expected action is to set a new style, different to the current one.
    void setChangeStyleCallback(std::function<void()> callback);

    void setShouldClose();

    void setWindowTitle(const std::string&);

    void run();
    void report(float duration);

private:
    mbgl::LatLng makeRandomPoint() const;
    static std::shared_ptr<const mbgl::SpriteImage>
    makeSpriteImage(int width, int height, float pixelRatio);

    void nextOrientation();

    void addRandomPointAnnotations(int count);
    void addRandomShapeAnnotations(int count);
    void addRandomCustomPointAnnotations(int count);

    void clearAnnotations();
    void popAnnotation();

    mbgl::AnnotationIDs annotationIDs;
    std::vector<std::string> spriteIDs;

    mbgl::util::RunLoop loop;

    bool fullscreen = false;
    const bool benchmark = false;
    bool tracking = false;
    bool rotating = false;
    bool pitching = false;

    // Frame timer
    int frames = 0;
    float frameTime = 0;
    double lastReported = 0;

    int width = 1024;
    int height = 768;
    int fbWidth;
    int fbHeight;
    float pixelRatio;

    double lastX = 0, lastY = 0;

    double lastClick = -1;

    std::function<void()> changeStyleCallback;

    GLFWwindow *window = nullptr;
    std::atomic_flag render = ATOMIC_FLAG_INIT;
    std::atomic_flag process = ATOMIC_FLAG_INIT;
};

#endif
