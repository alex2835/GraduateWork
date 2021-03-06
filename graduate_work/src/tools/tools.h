#pragma once

#include "bubble.h"
#include "pixel.h"
#include "cluster.h"
#include "cpu/image_cpu.h"
#include "gpu/image_gpu.h"
#include "random.hpp"
using Random = effolkronium::random_static;


glm::i8vec4 RandomColor()
{
    glm::i8vec4 res(4);
    for (int i = 0; i < 3; i++)
    {
        res[i] = Random::get<uint8_t>(0, 255);
    }
    res[3] = 255;
    return res;
}

inline std::vector<glm::i8vec4> GetRandomColors(int size)
{
    std::vector<glm::i8vec4> colors;
    for (int i = 0; i < size; i++)
    {
        colors.push_back(RandomColor());
    }
    return colors;
}


// ================= Image - Cluster data transfrormation ================= 

inline std::vector<Pixel> GetPixels(const cpu::Image& image)
{
    uint32_t image_size = image.GetWidth() * image.GetHeight();
    std::vector<Pixel> pixels;
    pixels.reserve(image_size);

    for (int y = 0; y < image.GetHeight(); y++)
    {
        for (int x = 0; x < image.GetWidth(); x++)
        {
            Pixel pixel = { x, y };
            const uint8_t* color = image.GetColor(x, y);
            for (int i = 0; i < image.GetChannels(); i++)
            {
                pixel.color[i] = color[i];
            }
            pixels.push_back(pixel);
        }
    }
    return std::move(pixels);
}


inline gpu::Image GetImageFromClusters(const std::vector<Cluster<Pixel>>& clusters, Texture2DSpecification spec)
{
    cpu::Image image(spec);
    auto colors = GetRandomColors(clusters.size());

    int sum = 0;

    for (int i = 0; i < clusters.size(); i++)
    {
        sum += clusters[i].Size();
        for (const Pixel& pixel : clusters[i])
        {
            auto color = colors[i];
            image.SetColor((uint8_t*)&color, pixel.x, pixel.y);
        }
    }
    BUBBLE_ASSERT(sum == spec.Width * spec.Height, "");
    return image.LoadOnGPU();
}

template <typename Point>
inline int GetClusterByPixel(const std::vector<Cluster<Pixel>>& clusters, Point point)
{
    for (int i = 0; i < clusters.size(); i++)
    {
        for (const Pixel& pixel_in_cluster : clusters[i])
        {
            if ((int)point.x == (int)pixel_in_cluster.x &&
                (int)point.y == (int)pixel_in_cluster.y)
            {
                return i;
            }
        }
    }
    BUBBLE_ASSERT(false, "Ti shto durak bliat");
    return -1;
}

inline gpu::Image GetImageFromCluster(const Cluster<Pixel>& cluster, Texture2DSpecification spec)
{
    cpu::Image image(spec);
    const uint8_t color[] = {255, 255, 255, 255};

    auto image_size     = image.GetWidth() * image.GetHeight();
    auto image_channels = image.GetChannels();
    memset(image.mData.get(), 0, image_size * image_channels);

    for (const Pixel& pixel : cluster)
    {
        image.SetColor(color, pixel.x, pixel.y);
    }
    return image.LoadOnGPU();
}

inline std::vector<Ref<gpu::Image>> GetRefImagesFromPixelData(const std::vector<std::vector<Pixel>>& snapshots, Texture2DSpecification spec)
{
    std::vector<Ref<gpu::Image>> images;
    for (const auto& snapshot : snapshots)
    {
        cpu::Image snapshot_image(spec);
        for (auto& pixel : snapshot)
        {
            uint8_t color[] = { (uint8_t)pixel.color[0], (uint8_t)pixel.color[1], (uint8_t)pixel.color[2], 255 };
            snapshot_image.SetColor(color, pixel.x, pixel.y);
        }
        images.push_back(CreateRef<gpu::Image>(snapshot_image.LoadOnGPU()));
    }
    return images;
}


// ================= Target value =================

int Metric(const Pixel& pixel, ImVec2 center, float radius)
{
    float distance = sqrt(pow(pixel.x - center.x, 2) + pow(pixel.y - center.y, 2));
    return distance <= radius ? 1 : -5;
}

inline int CalculateTargetValue(const std::vector<Cluster<Pixel>>& clusters, ImVec2 class_point, ImVec2 center, float radius)
{
    int cluster_id = GetClusterByPixel(clusters, class_point);

    int result = 0;
    for (const Pixel& pixel : clusters[cluster_id])
    {
        result += Metric(pixel, center, radius);
    }
    return result;
}