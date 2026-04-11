//
// Created by wsoll on 4/11/2026.
//

#ifndef NURBS_DDE_ICURVE_HPP
#define NURBS_DDE_ICURVE_HPP

#include "core/Types.hpp"


namespace ndde::app {
    class ICurve {
    public:
        virtual ~ICurve() = default;

        // The Core: Everything is 3D (z=0 for 2D)
        virtual glm::vec3 evaluate(float t) const = 0;

        // High-Order Analysis
        virtual glm::vec3 derivative(float t) const = 0; // 1st Derivative (Velocity)
        virtual glm::vec3 secondDerivative(float t) const = 0; // 2nd (Acceleration/Curvature)
        virtual glm::vec3 thirdDerivative(float t) const = 0; // 3rd (Jerk)

        // Metric Properties
        virtual float curvature(float t) const {
            auto d1 = derivative(t);
            auto d2 = secondDerivative(t);
            float len = glm::length(d1);
            // Cubing via multiplication is faster and stays in float-land
            return glm::length(glm::cross(d1, d2)) / (len * len * len);
        }

        virtual void generateVertices(std::vector<Vertex>& outBuffer) const = 0;
    };
}
#endif //NURBS_DDE_ICURVE_HPP
