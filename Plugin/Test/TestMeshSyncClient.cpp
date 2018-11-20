#include "pch.h"
#include "Test.h"
#include "MeshGenerator.h"
#include "../MeshSync/MeshSync.h"
#include "../MeshSync/MeshSyncUtils.h"
using namespace mu;

static void Send(ms::Scene& scene)
{
    for (auto& obj : scene.objects) {
        if (auto *mesh = dynamic_cast<ms::Mesh*>(obj.get())) {
            mesh->setupFlags();
        }
    }

    ms::ClientSettings settings;
    ms::Client client(settings);

    {
        ms::FenceMessage mes;
        mes.type = ms::FenceMessage::FenceType::SceneBegin;
        client.send(mes);
    }
    {
        ms::SetMessage mes;
        mes.scene = scene;
        client.send(mes);
    }
    {
        ms::FenceMessage mes;
        mes.type = ms::FenceMessage::FenceType::SceneEnd;
        client.send(mes);
    }
}

struct Random
{
    std::mt19937 r;
    std::uniform_real_distribution<float> d01, d11;

    Random()
    {
        r.seed(std::random_device()());
        d01 = std::uniform_real_distribution<float>(0.0f, 1.0f);
        d11 = std::uniform_real_distribution<float>(-1.0f, 1.0f);
    }

    float f01()
    {
        return d01(r);
    }

    float f11()
    {
        return d11(r);
    }

    float3 v3n()
    {
        return normalize(float3{ f11(), f11(), f11() });
    }
};



TestCase(Test_SendMesh)
{
    for (int i = 0; i < 8; ++i) {
        ms::Scene scene;

        auto mesh = ms::Mesh::create();
        scene.objects.push_back(mesh);

        mesh->path = "/Test/Wave";
        mesh->refine_settings.flags.gen_normals = 1;
        mesh->refine_settings.flags.gen_tangents = 1;


        auto& points = mesh->points;
        auto& uv = mesh->uv0;
        auto& counts = mesh->counts;
        auto& indices = mesh->indices;
        auto& mids = mesh->material_ids;

        GenerateWaveMesh(counts, indices, points, uv, 2.0f, 1.0f, 32, 30.0f * mu::Deg2Rad * i);
        mids.resize(counts.size(), 0);

        Send(scene);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}


TestCase(Test_Animation)
{
    ms::Scene scene;
    {
        auto node = ms::Mesh::create();
        scene.objects.push_back(node);

        node->path = "/Test/Animation";
        node->position = { 0.0f, 0.0f, 0.0f };
        node->rotation = quatf::identity();
        node->scale = { 1.0f, 1.0f, 1.0f };
        GenerateIcoSphereMesh(node->counts, node->indices, node->points, node->uv0, 0.5f, 1);
        node->refine_settings.flags.gen_normals = 1;
        node->refine_settings.flags.gen_tangents = 1;
    }
    {
        auto clip = ms::AnimationClip::create();
        scene.animations.push_back(clip);

        auto anim = ms::TransformAnimation::create();
        clip->animations.push_back(anim);

        anim->path = "/Test/Animation";
        anim->translation.push_back({ 0.0f, {0.0f, 0.0f, 0.0f} });
        anim->translation.push_back({ 1.0f, {1.0f, 0.0f, 0.0f} });
        anim->translation.push_back({ 2.0f, {1.0f, 1.0f, 0.0f} });
        anim->translation.push_back({ 3.0f, {1.0f, 1.0f, 1.0f} });

        anim->rotation.push_back({ 0.0f, ms::rotateX(0.0f * mu::Deg2Rad) });
        anim->rotation.push_back({ 1.0f, ms::rotateX(90.0f * mu::Deg2Rad) });
        anim->rotation.push_back({ 2.0f, ms::rotateX(180.0f * mu::Deg2Rad) });
        anim->rotation.push_back({ 3.0f, ms::rotateX(270.0f * mu::Deg2Rad) });

        anim->scale.push_back({ 0.0f, {1.0f, 1.0f, 1.0f} });
        anim->scale.push_back({ 1.0f, {2.0f, 2.0f, 2.0f} });
        anim->scale.push_back({ 2.0f, {1.0f, 1.0f, 1.0f} });
        anim->scale.push_back({ 3.0f, {2.0f, 2.0f, 2.0f} });
    }
    Send(scene);
}

TestCase(Test_Points)
{
    Random rand;

    ms::Scene scene;
    {
        auto node = ms::Mesh::create();
        scene.objects.push_back(node);

        node->path = "/Test/PointMesh";
        node->position = { 0.0f, 0.0f, 0.0f };
        node->rotation = quatf::identity();
        node->scale = { 1.0f, 1.0f, 1.0f };
        node->visible_hierarchy = false;
        GenerateIcoSphereMesh(node->counts, node->indices, node->points, node->uv0, 0.1f, 1);
        node->refine_settings.flags.gen_normals = 1;
        node->refine_settings.flags.gen_tangents = 1;
    }
    {
        auto node = ms::Points::create();
        scene.objects.push_back(node);

        node->path = "/Test/PointsT";
        node->reference = "/Test/PointMesh";
        node->position = { -2.5f, 0.0f, 0.0f };

        auto data = ms::PointsData::create();
        node->data.push_back(data);

        int c = 100;
        data->points.resize_discard(c);
        for (int i = 0; i < c;++i) {
            data->points[i] = { rand.f11(), rand.f11(), rand.f11() };
        }
        node->setupFlags();
    }
    {
        auto node = ms::Points::create();
        scene.objects.push_back(node);

        node->path = "/Test/PointsTR";
        node->reference = "/Test/PointMesh";
        node->position = { 0.0f, 0.0f, 0.0f };

        auto data = ms::PointsData::create();
        node->data.push_back(data);

        int c = 100;
        data->points.resize_discard(c);
        data->rotations.resize_discard(c);
        for (int i = 0; i < c; ++i) {
            data->points[i] = { rand.f11(), rand.f11(), rand.f11() };
            data->rotations[i] = rotate(rand.v3n(), rand.f11() * mu::PI);
        }
        node->setupFlags();
    }
    {
        auto node = ms::Points::create();
        scene.objects.push_back(node);

        auto clip = ms::AnimationClip::create();
        scene.animations.push_back(clip);

        auto anim = ms::PointsAnimation::create();
        clip->animations.push_back(anim);

        anim->path = "/Test/PointsTRS";
        node->path = "/Test/PointsTRS";
        node->reference = "/Test/PointMesh";
        node->position = { 2.5f, 0.0f, 0.0f };

        int num_points = 100;
        int num_frames = 100;

        RawVector<float3> points(num_points);
        RawVector<quatf> rotations(num_points);
        RawVector<float3> scales(num_points);
        RawVector<float3> velocities(num_points);
        RawVector<float4> colors(num_points);

        for (int i = 0; i < num_points; ++i) {
            points[i] = { rand.f11(), rand.f11(), rand.f11() };
            rotations[i] = rotate(rand.v3n(), rand.f11() * mu::PI);
            scales[i] = { rand.f01(), rand.f01(), rand.f01() };
            velocities[i] = float3{ rand.f11(), rand.f11(), rand.f11() } *0.1f;
            colors[i] = { rand.f01(), rand.f01(), rand.f01() };
        }

        for (int fi = 0; fi < num_frames; ++fi) {
            auto data = ms::PointsData::create();
            node->data.push_back(data);

            float t = (1.0f / 30.0f) * fi;
            anim->time.push_back({ t, {t} });
            data->time = t;
            data->points = points;
            data->rotations = rotations;
            data->scales = scales;
            data->velocities = velocities;
            data->colors = colors;
            for (int i = 0; i < num_points; ++i) {
                points[i] += velocities[i];
            }
        }
        node->setupFlags();
    }

    Send(scene);
}


template<class color_t>
void CreateCheckerImage(RawVector<char>& dst, color_t black, color_t white, int width, int height)
{
    int num_pixels = width * height;
    int checker_size = 8;
    dst.resize_discard(num_pixels * sizeof(color_t));
    color_t *data = (color_t*)dst.data();
    for (int iy = 0; iy < height; ++iy) {
        for (int ix = 0; ix < width; ++ix) {
            bool cy = (iy / checker_size) % 2 == 0;
            bool cx = (ix / checker_size) % 2 == 0;
            if (cy)
                *data++ = cx ? white : black;
            else
                *data++ = cx ? black : white;
        }
    }
}

template<class color_t>
ms::TexturePtr CreateCheckerImageTexture(color_t black, color_t white, int width, int height, int id, const char *name)
{
    auto tex = ms::Texture::create();
    tex->id = id;
    tex->name = name;
    tex->format = ms::GetTextureFormat<color_t>::result;
    tex->width = width;
    tex->height = height;
    CreateCheckerImage(tex->data, black, white, width, height);
    return tex;
}

TestCase(Test_SendTexture)
{
    auto gen_id = []() {
        static int id_seed = 0;
        return ++id_seed;
    };

    // raw file textures
    {
        const char *raw_files[] = {
            "Texture_RGBA_u8.png",
            "Texture_RGBA_f16.exr",
        };

        ms::Scene scene;
        for (auto filename : raw_files) {
            auto tex = ms::Texture::create();
            if (tex->readFromFile(filename)) {
                scene.textures.push_back(tex);
                tex->id = gen_id();
            }
        }
        if (!scene.textures.empty())
            Send(scene);
    }

    {
        ms::Scene scene;

        const int width = 512;
        const int height = 512;
        {
            // Ru8
            unorm8 black{ 0.0f };
            unorm8 white{ 1.0f };
            scene.textures.push_back(CreateCheckerImageTexture(black, white, width, height, gen_id(), "Ru8"));
        }
        {
            // RGu8
            unorm8x2 black{ 0.0f, 0.0f };
            unorm8x2 white{ 1.0f, 1.0f };
            scene.textures.push_back(CreateCheckerImageTexture(black, white, width, height, gen_id(), "RGu8"));
        }
        {
            // RGBAu8
            unorm8x3 black{ 0.0f, 0.0f, 0.0f };
            unorm8x3 white{ 1.0f, 1.0f, 1.0f };
            scene.textures.push_back(CreateCheckerImageTexture(black, white, width, height, gen_id(), "RGBu8"));
        }
        {
            // RGBAu8
            unorm8x4 black{ 0.0f, 0.0f, 0.0f, 1.0f };
            unorm8x4 white{ 1.0f, 1.0f, 1.0f, 1.0f };
            scene.textures.push_back(CreateCheckerImageTexture(black, white, width, height, gen_id(), "RGBAu8"));
        }
        {
            // RGBAf16
            half4 black{ 0.0f, 0.0f, 0.0f, 1.0f };
            half4 white{ 1.0f, 1.0f, 1.0f, 1.0f };
            scene.textures.push_back(CreateCheckerImageTexture(black, white, width, height, gen_id(), "RGBAf16"));
        }
        {
            // RGBAf32
            float4 black{ 0.0f, 0.0f, 0.0f, 1.0f };
            float4 white{ 1.0f, 1.0f, 1.0f, 1.0f };
            scene.textures.push_back(CreateCheckerImageTexture(black, white, width, height, gen_id(), "RGBAf32"));
        }

        // material
        {
            auto mat = ms::Material::create();
            scene.materials.push_back(mat);
            mat->name = "TestMaterial1";
            mat->id = 0;
            auto& stdmat = ms::AsStandardMaterial(*mat);
            stdmat.setColor({ 0.3f, 0.3f, 0.5f, 1.0f });
            stdmat.setEmissionColor({ 0.7f, 0.1f, 0.2f, 1.0f });
            stdmat.setMetallic(0.2f);
            stdmat.setSmoothness(0.8f);
            stdmat.setColorMap(1);
            stdmat.setMetallicMap(5);
            stdmat.setEmissionMap(4);

            stdmat.addKeyword({ "_EMISSION", true });
            stdmat.addKeyword({ "_INVALIDKEYWORD", true });
        }
        Send(scene);
    }

    {
        ms::Scene scene;

        // file asset
        {
            auto as = ms::FileAsset::create();
            if (as->readFromFile("explosion1.wav"))
                scene.assets.push_back(as);
        }
        Send(scene);
    }
}


TestCase(Test_Query)
{
    auto send_query_impl = [](ms::QueryMessage::QueryType qt, const char *query_name) {
        ms::ClientSettings settings;
        ms::Client client(settings);

        ms::QueryMessage query;
        query.type = qt;
        auto response = std::dynamic_pointer_cast<ms::ResponseMessage>(client.send(query));

        printf("querty: %s\n", query_name);
        printf("response:\n");
        if (response) {
            for (auto& t : response->text)
                printf("  %s\n", t.c_str());
        }
        else {
            printf("  (null)\n");
        }
    };

#define SendQuery(Q) send_query_impl(Q, #Q)
    SendQuery(ms::QueryMessage::QueryType::ClientName);
    SendQuery(ms::QueryMessage::QueryType::RootNodes);
    SendQuery(ms::QueryMessage::QueryType::AllNodes);
#undef SendQuery
}
