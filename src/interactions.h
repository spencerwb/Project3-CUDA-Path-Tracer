#pragma once

#include "intersections.h"

// CHECKITOUT
/**
 * Computes a cosine-weighted random direction in a hemisphere.
 * Used for diffuse lighting.
 */
__host__ __device__
glm::vec3 calculateRandomDirectionInHemisphere(
        glm::vec3 normal, thrust::default_random_engine &rng) {
    thrust::uniform_real_distribution<float> u01(0, 1);

    float up = sqrt(u01(rng)); // cos(theta)
    float over = sqrt(1 - up * up); // sin(theta)
    float around = u01(rng) * TWO_PI;

    // Find a direction that is not the normal based off of whether or not the
    // normal's components are all equal to sqrt(1/3) or whether or not at
    // least one component is less than sqrt(1/3). Learned this trick from
    // Peter Kutz.

    glm::vec3 directionNotNormal;
    if (abs(normal.x) < SQRT_OF_ONE_THIRD) {
        directionNotNormal = glm::vec3(1, 0, 0);
    } else if (abs(normal.y) < SQRT_OF_ONE_THIRD) {
        directionNotNormal = glm::vec3(0, 1, 0);
    } else {
        directionNotNormal = glm::vec3(0, 0, 1);
    }

    // Use not-normal direction to generate two perpendicular directions
    glm::vec3 perpendicularDirection1 =
        glm::normalize(glm::cross(normal, directionNotNormal));
    glm::vec3 perpendicularDirection2 =
        glm::normalize(glm::cross(normal, perpendicularDirection1));

    return up * normal
        + cos(around) * over * perpendicularDirection1
        + sin(around) * over * perpendicularDirection2;
}

/**
 * Scatter a ray with some probabilities according to the material properties.
 * For example, a diffuse surface scatters in a cosine-weighted hemisphere.
 * A perfect specular surface scatters in the reflected ray direction.
 * In order to apply multiple effects to one surface, probabilistically choose
 * between them.
 * 
 * The visual effect you want is to straight-up add the diffuse and specular
 * components. You can do this in a few ways. This logic also applies to
 * combining other types of materials (such as refractive).
 * 
 * - Always take an even (50/50) split between a each effect (a diffuse bounce
 *   and a specular bounce), but divide the resulting color of either branch
 *   by its probability (0.5), to counteract the chance (0.5) of the branch
 *   being taken.
 *   - This way is inefficient, but serves as a good starting point - it
 *     converges slowly, especially for pure-diffuse or pure-specular.
 * - Pick the split based on the intensity of each material color, and divide
 *   branch result by that branch's probability (whatever probability you use).
 *
 * This method applies its changes to the Ray parameter `ray` in place.
 * It also modifies the color `color` of the ray in place.
 *
 * You may need to change the parameter list for your purposes!
 */
__host__ __device__
void scatterRay(
    const Camera& cam,
    thrust::default_random_engine& rng,
	PathSegment& pathSegment,
    ShadeableIntersection& intersection,
    Material& mat,
    const int* lights,
    int lightsNum,
    const Geom* geoms,
    Material* materials)
{
    // TODO: implement this.
    // A basic implementation of pure-diffuse shading will just call the
    // calculateRandomDirectionInHemisphere defined above.
    glm::vec3 color;
    glm::vec3 normal = intersection.surfaceNormal;
    glm::vec3 wi = -pathSegment.ray.direction;
    pathSegment.ray.origin = intersection.intersectPos;

    thrust::uniform_real_distribution<float> u01(0, 1);
    float randMat = u01(rng);

    // computing which light to sample from
    // and its relevant features
    float rL = u01(rng) * lightsNum;
    int randLight = (int)rL;
    glm::vec3 L = geoms[lights[randLight]].translation - pathSegment.ray.origin;
    L = glm::normalize(L);

    // used 2nd suggestion for determining which material to sample
    if (randMat <= mat.hasReflective) {
        // specular calculation
        glm::vec3 r = glm::reflect(pathSegment.ray.direction, normal);
        glm::vec3 v = glm::normalize(intersection.intersectPos);
        color = glm::vec3(1.f) * mat.specular.color * mat.hasReflective * glm::pow(glm::dot(r, v), mat.specular.exponent) / mat.hasReflective;
        // glm::vec3 v = glm::normalize(cam.position - intersection.intersectPos);
        // color = glm::vec3(1.f) * mat.specular.color * mat.hasReflective * glm::pow(glm::dot(r, v), mat.specular.exponent) / mat.hasReflective;
        color = mat.specular.color;
        pathSegment.ray.direction = r;
    }
    else {
        // diffuse calculation
        float diffuse = glm::dot(normal, L);
        float ambient = 0.2;
        float lux = diffuse + ambient;
        color = pathSegment.throughput * mat.color * lux / (1.f - mat.hasReflective);
        // color = mat.color * pathSegment.throughput;
        color = mat.color;

        // update throughput
        pathSegment.throughput *= color * glm::abs(glm::dot(pathSegment.ray.direction, normal)) / TWO_PI;
        // update light bounce direction
        pathSegment.ray.direction = calculateRandomDirectionInHemisphere(normal, rng);
    }

    pathSegment.color = color;
    pathSegment.remainingBounces--;
    // pathSegment.remainingBounces = 0;
}


// Ray Tracer Code...
// Also make sure to uncomment Option 1 in shadeFakeMaterial
/*
__host__ __device__
void scatterRay(
    const Camera& cam,
    thrust::default_random_engine& rng,
    PathSegment& pathSegment,
    ShadeableIntersection& intersection,
    Material& mat,
    const int* lights,
    int lightsNum,
    const Geom* geoms,
    Material* materials)
{
    // TODO: implement this.
    // A basic implementation of pure-diffuse shading will just call the
    // calculateRandomDirectionInHemisphere defined above.
    glm::vec3 color;
    glm::vec3 normal = intersection.surfaceNormal;
    glm::vec3 wi = -pathSegment.ray.direction;
    pathSegment.ray.origin = intersection.intersectPos;

    thrust::uniform_real_distribution<float> u01(0, 1);
    float randMat = u01(rng);

    // computing which light to sample from
    // and its relevant features
    float rL = u01(rng) * lightsNum;
    int randLight = (int)rL;
    glm::vec3 L = geoms[lights[randLight]].translation - pathSegment.ray.origin;
    L = glm::normalize(L);

    // used 2nd suggestion for determining which material to sample
    if (randMat <= mat.hasReflective) {
        // specular calculation
        glm::vec3 r = glm::reflect(pathSegment.ray.direction, normal);
        glm::vec3 v = glm::normalize(intersection.intersectPos);
        color = glm::vec3(1.f) * mat.specular.color * mat.hasReflective * glm::pow(glm::dot(r, v), mat.specular.exponent) / mat.hasReflective;
        // glm::vec3 v = glm::normalize(cam.position - intersection.intersectPos);
        // color = glm::vec3(1.f) * mat.specular.color * mat.hasReflective * glm::pow(glm::dot(r, v), mat.specular.exponent) / mat.hasReflective;
        color = mat.specular.color;
        pathSegment.ray.direction = r;
    }
    else {
        // diffuse calculation
        float diffuse = glm::dot(normal, L);
        float ambient = 0.2;
        float lux = diffuse + ambient;
        color = pathSegment.throughput * mat.color * lux / (1.f - mat.hasReflective);
        // color = mat.color * pathSegment.throughput;

        // update throughput
        pathSegment.throughput *= color * glm::abs(glm::dot(pathSegment.ray.direction, normal)) / TWO_PI;
        // update light bounce direction
        pathSegment.ray.direction = calculateRandomDirectionInHemisphere(normal, rng);
    }

    pathSegment.color = color;
    pathSegment.remainingBounces--;
    pathSegment.remainingBounces = 0;
}
*/