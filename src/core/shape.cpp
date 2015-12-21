/**
 This file is part of Deformable Shape Tracking (DEST).
 
 Copyright Christoph Heindl 2015
 
 Deformable Shape Tracking is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 Deformable Shape Tracking is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with Deformable Shape Tracking. If not, see <http://www.gnu.org/licenses/>.
 */

#include <dest/core/shape.h>
#include <Eigen/Dense>
#include <iostream>

namespace dest {
    namespace core {
        
        Eigen::AffineCompact2f estimateSimilarityTransform(const Shape &from, const Shape &to) {
            
            Eigen::Vector2f meanFrom = from.rowwise().mean();
            Eigen::Vector2f meanTo = to.rowwise().mean();
            
            Shape centeredFrom = from.colwise() - meanFrom;
            Shape centeredTo = to.colwise() - meanTo;
            
            Eigen::Matrix2f cov = (centeredFrom) * (centeredTo).transpose();
            cov /= static_cast<float>(from.cols());
            const float sFrom = centeredFrom.squaredNorm() / from.cols();
            
            auto svd = cov.jacobiSvd(Eigen::ComputeFullU | Eigen::ComputeFullV);
            Eigen::Matrix2f d = Eigen::Matrix2f::Zero(2, 2);
            d(0, 0) = svd.singularValues()(0);
            d(1, 1) = svd.singularValues()(1);
            
            // Correct reflection if any.
            float detCov = cov.determinant();
            float detUV = svd.matrixU().determinant() * svd.matrixV().determinant();
            Eigen::Matrix2f s = Eigen::Matrix2f::Identity(2, 2);
            if (detCov < 0.f || (detCov == 0.f && detUV < 0.f)) {
                if (svd.singularValues()(1) < svd.singularValues()(0)) {
                    s(1, 1) = -1;
                } else {
                    s(0, 0) = -1;
                }
            }
            
            Eigen::Matrix2f rot = svd.matrixU().transpose() * s * svd.matrixV();
            float c = 1.f;
            if (sFrom > 0) {
                c = 1.f / sFrom * (d * s).trace();
            }
            
            Eigen::Vector2f t = meanTo - c * rot * meanFrom;
            
            Eigen::Matrix<float, 2, 3> ret = Eigen::Matrix<float, 2, 3>::Identity(2, 3);
            ret.block<2,2>(0,0) = c * rot;
            ret.block<2,1>(0,2) = t;
            
            return Eigen::AffineCompact2f(ret);
        }
        
        int findClosestLandmarkIndex(const Shape &s, const Eigen::Ref<const Eigen::Vector2f> &x)
        {
            const int numLandmarks = static_cast<int>(s.cols());
            
            int bestLandmark = -1;
            float bestD2 = std::numeric_limits<float>::max();
            
            for (int i = 0; i < numLandmarks; ++i) {
                float d2 = (s.col(i) - x).squaredNorm();
                if (d2 < bestD2) {
                    bestD2 = d2;
                    bestLandmark = i;
                }
            }
            
            return bestLandmark;
        }
        
        
        void shapeRelativePixelCoordinates(const Shape &s, const PixelCoordinates &abscoords, PixelCoordinates &relcoords, Eigen::VectorXi &closestLandmarks)
        {
            
            relcoords.resize(abscoords.rows(), abscoords.cols());
            closestLandmarks.resize(abscoords.cols());
            
            const int numLocs = static_cast<int>(abscoords.cols());
            for (int i  = 0; i < numLocs; ++i) {
                int idx = findClosestLandmarkIndex(s, abscoords.col(i));
                relcoords.col(i) = abscoords.col(i) - s.col(idx);
                closestLandmarks(i) = idx;
            }
            
        }

    }
}