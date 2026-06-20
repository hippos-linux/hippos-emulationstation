#pragma once

#include "GuiComponent.h"
#include "components/MenuComponent.h"
#include "components/BusyComponent.h"

#include <functional>
#include <thread>

class GuiInstall : public GuiComponent {
public:
    // Generic constructor: op is called on a background thread.
    GuiInstall(Window* window, std::function<std::pair<std::string,int>(BusyComponent*)> op);

    // Convenience: full-disk install (device + architecture).
    GuiInstall(Window* window, std::string storageDevice, std::string architecture);

    virtual ~GuiInstall();

    void render(const Transform4x4f& parentTrans) override;
    bool input(InputConfig* config, Input input) override;
    std::vector<HelpPrompt> getHelpPrompts() override;
    void update(int deltaTime) override;

private:
    BusyComponent mBusyAnim;
    bool mLoading;
    int  mState;
    std::pair<std::string, int> mResult;

    std::function<std::pair<std::string,int>(BusyComponent*)> mOp;
    std::thread* mHandle;

    void onInstallOk();
    void onInstallError(std::pair<std::string, int>);
    void threadInstall();
};
