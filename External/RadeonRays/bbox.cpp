/**********************************************************************
Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
********************************************************************/

#include "bbox.h"

namespace RadeonRays
{
	Vec3 bbox::center()  const { return (pmax + pmin) * 0.5f; }
	Vec3 bbox::extents() const { return pmax - pmin; }

	float bbox::surface_area() const
	{
		Vec3 ext = extents();
		return 2.f * (ext.x * ext.y + ext.x * ext.z + ext.y * ext.z);
	}

	// Grow the bounding box by a point
	void bbox::grow(Vec3 const& p)
	{
		pmin = vec3Min(pmin, p);
		pmax = vec3Max(pmax, p);
	}
	// Grow the bounding box by a box
	void bbox::grow(bbox const& b)
	{
		pmin = vec3Min(pmin, b.pmin);
		pmax = vec3Max(pmax, b.pmax);
	}

	bool bbox::contains(Vec3 const& p) const
	{
		Vec3 radius = extents() * 0.5f;
		return std::abs(center().x - p.x) <= radius.x &&
			fabs(center().y - p.y) <= radius.y &&
			fabs(center().z - p.z) <= radius.z;
	}

	bbox bboxunion(bbox const& box1, bbox const& box2)
	{
		bbox res;
		res.pmin = vec3Min(box1.pmin, box2.pmin);
		res.pmax = vec3Max(box1.pmax, box2.pmax);
		return res;
	}

	bbox intersection(bbox const& box1, bbox const& box2)
	{
		return bbox(vec3Max(box1.pmin, box2.pmin), vec3Min(box1.pmax, box2.pmax));
	}

	void intersection(bbox const& box1, bbox const& box2, bbox& box)
	{
		box.pmin = vec3Max(box1.pmin, box2.pmin);
		box.pmax = vec3Min(box1.pmax, box2.pmax);
	}

	#define BBOX_INTERSECTION_EPS 0.f

	bool intersects(bbox const& box1, bbox const& box2)
	{
		Vec3 b1c = box1.center();
		Vec3 b1r = box1.extents() * 0.5f;
		Vec3 b2c = box2.center();
		Vec3 b2r = box2.extents() * 0.5f;

		return (fabs(b2c.x - b1c.x) - (b1r.x + b2r.x)) <= BBOX_INTERSECTION_EPS &&
			(fabs(b2c.y - b1c.y) - (b1r.y + b2r.y)) <= BBOX_INTERSECTION_EPS &&
			(fabs(b2c.z - b1c.z) - (b1r.z + b2r.z)) <= BBOX_INTERSECTION_EPS;
	}

	bool contains(bbox const& box1, bbox const& box2)
	{
		return box1.contains(box2.pmin) && box1.contains(box2.pmax);
	}

	// Only linear transformations can be processed
    bbox transformBy(const Mat4 &matrix, const bbox &box)
    {
		Vec3 minBound = box.pmin, maxBound = box.pmax;
		
		Vec3 right = Vec3(matrix[0][0], matrix[0][1], matrix[0][2]),
			up = Vec3(matrix[1][0], matrix[1][1], matrix[1][2]),
			forward = Vec3(matrix[2][0], matrix[2][1], matrix[2][2]),
			translation = Vec3(matrix[3][0], matrix[3][1], matrix[3][2]);

		Vec3 xa = right * minBound.x;
		Vec3 xb = right * maxBound.x;

		Vec3 ya = up * minBound.y;
		Vec3 yb = up * maxBound.y;

		Vec3 za = forward * minBound.z;
		Vec3 zb = forward * maxBound.z;

		// x_ = vec3(A00x, A10x, A20x) y_ = vec3(A01y, A02y, A03y) z_ = vec3(A02z, A12z, A22Z) translation = vec3(A03, A13, A23)
		// Mat \cdot P = Vec4(A00x + A01y + A02z + A03, A10x + A11y + A12z + A13, A20x + A21y + A22z + A23, 1)
		minBound = vec3Min(xa, xb) + vec3Min(ya, yb) + vec3Min(za, zb) + translation;
		maxBound = vec3Max(xa, xb) + vec3Max(ya, yb) + vec3Max(za, zb) + translation;

        return bbox(minBound, maxBound);
    }
    glm::vec3 vec3Min(const glm::vec3 &p1, const glm::vec3 &p2)
    {
        return glm::vec3(std::min(p1.x, p2.x), std::min(p1.y, p2.y), std::min(p1.z, p2.z));
    }
    glm::vec3 vec3Max(const glm::vec3 &p1, const glm::vec3 &p2)
    {
        return glm::vec3(std::max(p1.x, p2.x), std::max(p1.y, p2.y), std::max(p1.z, p2.z));
    }
}
