#include "pch.h"
#include "msAsyncSceneSender.h"

namespace ms {

AsyncSceneSender::AsyncSceneSender()
{
}

AsyncSceneSender::~AsyncSceneSender()
{
    wait();
}

bool AsyncSceneSender::isSending()
{
    if (m_future.valid() && m_future.wait_for(std::chrono::milliseconds(0)) == std::future_status::timeout)
        return true;
    return false;
}

void AsyncSceneSender::wait()
{
    if (m_future.valid()) {
        m_future.wait();
        m_future = {};
    }
}

void AsyncSceneSender::kick()
{
    wait();
    m_future = std::async(std::launch::async, [this]() { send(); });
}

void AsyncSceneSender::send()
{
    if (on_prepare)
        on_prepare();

    // ignore material-only scene for now
    if (textures.empty() && /*materials.empty() &&*/ transforms.empty() && geometries.empty() && animations.empty() && deleted.empty())
        return;

    std::sort(transforms.begin(), transforms.end(), [](TransformPtr& a, TransformPtr& b) { return a->order < b->order; });
    std::sort(geometries.begin(), geometries.end(), [](TransformPtr& a, TransformPtr& b) { return a->order < b->order; });

    ms::Client client(client_settings);

    // notify scene begin
    {
        ms::FenceMessage fence;
        fence.type = ms::FenceMessage::FenceType::SceneBegin;
        client.send(fence);
    }

    // textures
    if (!textures.empty()) {
        for (auto& tex : textures) {
            ms::SetMessage set;
            set.scene.settings = scene_settings;
            set.scene.textures = { tex };
            client.send(set);
        };
        textures.clear();
    }

    // materials and non-geometry objects
    if (!materials.empty() || !transforms.empty()) {
        ms::SetMessage set;
        set.scene.settings = scene_settings;
        set.scene.materials = materials;
        set.scene.objects = transforms;
        client.send(set);

        textures.clear();
        materials.clear();
        transforms.clear();
    }

    // geometries
    if (!geometries.empty()) {
        for (auto& geom : geometries) {
            ms::SetMessage set;
            set.scene.settings = scene_settings;
            set.scene.objects = { geom };
            client.send(set);
        };
        geometries.clear();
    }

    // animations
    if (!animations.empty()) {
        ms::SetMessage set;
        set.scene.settings = scene_settings;
        set.scene.animations = animations;
        client.send(set);

        animations.clear();
    }

    // deleted
    if (!deleted.empty()) {
        ms::DeleteMessage del;
        del.targets = deleted;
        client.send(del);

        deleted.clear();
    }

    // notify scene end
    {
        ms::FenceMessage fence;
        fence.type = ms::FenceMessage::FenceType::SceneEnd;
        bool succeeded = client.send(fence);

        if (succeeded) {
            if (on_succeeded)
                on_succeeded();
        }
        else {
            if (on_failed)
                on_failed();
        }
    }
}

} // namespace ms

