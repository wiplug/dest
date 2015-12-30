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

#include <dest/dest.h>

#include <dest/io/database_io.h>
#include <dest/face/face_detector.h>
#include <dest/util/draw.h>
#include <dest/util/convert.h>
#include <iostream>
#include <opencv2/opencv.hpp>

#include <tclap/CmdLine.h>

int main(int argc, char **argv)
{
    struct {
        dest::core::TrainingParameters trainingParams;
        dest::core::SampleCreationParameters createParams;
        dest::io::ImportParameters importParams;
        std::string db;
        std::string rects;
        std::string output;
    } opts;

    try {
        TCLAP::CmdLine cmd("Train cascade of regressors using a landmark database and initial rectangles.", ' ', "0.9");
        
        TCLAP::ValueArg<int> numCascadesArg("", "train-num-cascades", "Number of cascades to train.", false, 10, "int", cmd);
        TCLAP::ValueArg<int> numTreesArg("", "train-num-trees", "Number of trees per cascade.", false, 500, "int", cmd);
        TCLAP::ValueArg<int> maxTreeDepthArg("", "train-max-depth", "Maximum tree depth.", false, 5, "int", cmd);
        TCLAP::ValueArg<int> numPixelsArg("", "train-num-pixels", "Number of random pixel coordinates", false, 400, "int", cmd);
        TCLAP::ValueArg<int> numSplitTestsArg("", "train-num-splits", "Number of random split tests at each tree node", false, 20, "int", cmd);
        TCLAP::ValueArg<float> lambdaArg("", "train-lambda", "Prior that favors closer pixel coordinates.", false, 0.1f, "float", cmd);
        TCLAP::ValueArg<float> learnArg("", "train-learn", "Learning rate of each tree.", false, 0.08f, "float", cmd);
        
        TCLAP::ValueArg<int> numShapesPerImageArg("", "create-num-shapes", "Number of shapes per image to create.", false, 20, "int", cmd);
        TCLAP::SwitchArg noCombinationsArg("", "create-no-combinations", "Disable linear combinations of shapes.", cmd, false);
        
        
        TCLAP::ValueArg<std::string> rectsArg("r", "rectangles", "Initial detection rectangles to train on.", true, "rectangles.csv", "string", cmd);
        TCLAP::ValueArg<std::string> outputArg("o", "output", "Trained cascade of regressors file.", false, "dest.bin", "string", cmd);
        TCLAP::ValueArg<int> maxImageSizeArg("", "load-max-size", "Maximum size of images in the database", false, 2048, "int", cmd);
        TCLAP::UnlabeledValueArg<std::string> databaseArg("database", "Path to database directory to load", true, "./db", "string", cmd);


        cmd.parse(argc, argv);
        
        opts.createParams.numShapesPerImage = numShapesPerImageArg.getValue();
        opts.createParams.numTransformPertubationsPerShape = 0;
        opts.createParams.useLinearCombinationsOfShapes = !noCombinationsArg.getValue();

        opts.trainingParams.numCascades = numCascadesArg.getValue();
        opts.trainingParams.numTrees = numTreesArg.getValue();
        opts.trainingParams.maxTreeDepth = maxTreeDepthArg.getValue();
        opts.trainingParams.numRandomPixelCoordinates = numPixelsArg.getValue();
        opts.trainingParams.numRandomSplitTestsPerNode = numSplitTestsArg.getValue();
        opts.trainingParams.exponentialLambda = lambdaArg.getValue();
        opts.trainingParams.learningRate = learnArg.getValue();
        
        opts.importParams.maxImageSideLength = maxImageSizeArg.getValue();
        
    
        opts.db = databaseArg.getValue();
        opts.rects = rectsArg.getValue();
        opts.output = outputArg.getValue();        
    }
    catch (TCLAP::ArgException &e) {
        std::cerr << "Error: " << e.error() << " for arg " << e.argId() << std::endl;
        return -1;
    }

    dest::core::InputData inputs;
    if (!dest::io::importDatabase(opts.db, opts.rects, inputs.images, inputs.shapes, inputs.rects, opts.importParams)) {
        std::cerr << "Failed to load database." << std::endl;
        return -1;
    }

    dest::core::InputData::normalizeShapes(inputs);
    dest::core::InputData validation;
    dest::core::InputData::randomPartition(inputs, validation, 0.01f);
    
    dest::core::TrainingData td(inputs);
    td.params = opts.trainingParams;
    dest::core::TrainingData::createTrainingSamples(td, opts.createParams);

    dest::core::Tracker t;
    t.fit(td);
    
    std::cout << "Saving tracker to " << opts.output << std::endl;
    t.save(opts.output);
    
    dest::core::TrainingData tdValidation(validation);
    dest::core::SampleCreationParameters validationCreateParams;
    validationCreateParams.numShapesPerImage = 1;
    validationCreateParams.numTransformPertubationsPerShape = 10;
    validationCreateParams.useLinearCombinationsOfShapes = false;
    
    dest::core::TrainingData::createTrainingSamples(tdValidation, validationCreateParams);
    for (size_t i = 0; i < tdValidation.samples.size(); ++i) {
        dest::core::TrainingData::Sample &s = tdValidation.samples[i];
        
        dest::core::Shape shape = t.predict(validation.images[s.inputIdx], s.shapeToImage);
        cv::Mat tmp = dest::util::drawShape(validation.images[s.inputIdx], shape, cv::Scalar(0, 255, 0));
        cv::imshow("result", tmp);
        cv::waitKey();
    }


    return 0;
}
