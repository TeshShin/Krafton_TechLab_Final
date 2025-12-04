#pragma once
#include "SViewerWindow.h"
#include "Source/Runtime/Engine/Viewer/ViewerState.h"

class FViewport;
class FViewportClient;
class UWorld;
struct ID3D11Device;

class SSkeletalMeshViewerWindow : public SViewerWindow
{
public:
    SSkeletalMeshViewerWindow();
    virtual ~SSkeletalMeshViewerWindow();

    virtual void OnRender() override;
    virtual void PreRenderViewportUpdate() override;

    // Skeletal Mesh Viewer는 파일 작업 비활성화
    virtual bool IsSaveEnabled() const override { return false; }
    virtual bool IsSaveAsEnabled() const override { return false; }
    virtual bool IsLoadEnabled() const override { return false; }

protected:
    virtual ViewerState* CreateViewerState(const char* Name, UEditorAssetPreviewContext* Context) override;
    virtual void DestroyViewerState(ViewerState*& State) override;
    virtual FString GetWindowTitle() const override { return "Skeletal Mesh Viewer"; }

private:
    // Load a skeletal mesh into the active tab
    void LoadSkeletalMesh(ViewerState* State, const FString& Path);

    // ImGui draw callback for viewport rendering
    static void ViewportRenderCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd);
};