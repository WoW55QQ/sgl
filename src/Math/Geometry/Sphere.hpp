/*!
 * Sphere.hpp
 *
 *  Created on: 10.09.2017
 *      Author: Christoph Neuhauser
 */

#ifndef SRC_MATH_GEOMETRY_SPHERE_HPP_
#define SRC_MATH_GEOMETRY_SPHERE_HPP_

#include <glm/glm.hpp>
#include <cfloat>
#include <Defs.hpp>

namespace sgl {

class DLL_OBJECT Sphere {
public:
    Sphere() : radius(0.0f) {}
    Sphere(glm::vec3 center, float radius) : center(center), radius(radius) {}
    glm::vec3 center;
    float radius;
};

}

/*! SRC_MATH_GEOMETRY_SPHERE_HPP_ */
#endif
