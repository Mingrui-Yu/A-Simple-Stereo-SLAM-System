#include "myslam/system.h"

#include "myslam/config.h"
#include "myslam/keyframe.h"
#include "myslam/map.h"

namespace myslam{

// -------------------------------------------------------------------------

System::System(const std::string &strConfigPath):
        _strConfigPath(strConfigPath) {}



// -------------------------------------------------------------------------

bool System::Init(){

    // read the config file
    if (Config::SetParameterFile(_strConfigPath) == false){
        return false;
    }

    // initial the ORB extractor, for frontend and loopclosing
    int numORBNewFeatures = Config::Get<int>("ORBextractor.nNewFeatures");
    float fScaleFactor = Config::Get<float>("ORBextractor.scaleFactor");
    int nLevels = Config::Get<int>("ORBextractor.nLevels");
    int fIniThFAST = Config::Get<int>("ORBextractor.iniThFAST");
    int fMinThFAST = Config::Get<int>("ORBextractor.minThFAST");
    _mpORBextractor = ORBextractor::Ptr(new ORBextractor(numORBNewFeatures,fScaleFactor,nLevels,fIniThFAST,fMinThFAST));

    // get left and right cameras
     _mbNeedUndistortion = Config::Get<int>("Camera.bNeedUndistortion");
    GetCamera();

    // create components and start each one's thread
    _mpFrontend = Frontend::Ptr(new Frontend);
    _mpBackend = Backend::Ptr(new Backend);
    _mpLoopClosing = LoopClosing::Ptr(new LoopClosing);
    _mpMap = Map::Ptr(new Map);

    bool bUseViewer = Config::Get<int>("Viewer.bShow");
    if(bUseViewer){
        _mpViewer = Viewer::Ptr(new Viewer);
    }
   

    // create links between each components
    _mpFrontend->SetViewer(_mpViewer);
    _mpFrontend->SetCameras(_mpCameraLeft, _mpCameraRight);
    _mpFrontend->SetMap(_mpMap);
    _mpFrontend->SetBackend(_mpBackend);
    _mpFrontend->SetORBextractor(_mpORBextractor);

    if(_mpBackend){
        _mpBackend->SetMap(_mpMap);
        _mpBackend->SetViewer(_mpViewer);
        _mpBackend->SetCameras(_mpCameraLeft, _mpCameraRight);
        _mpBackend->SetLoopClosing(_mpLoopClosing);
    }

    if(_mpLoopClosing){
        _mpLoopClosing->SetMap(_mpMap);
        _mpLoopClosing->SetCameras(_mpCameraLeft, _mpCameraRight);
        _mpLoopClosing->SetORBextractor(_mpORBextractor);
        _mpLoopClosing->SetBackend(_mpBackend);
    }

    if(_mpViewer){
        _mpViewer->SetMap(_mpMap);
    }
    

    return true;
}

// ----------------------------------------------------------------------------------------------

void System::Stop(){
    if(_mpViewer)
        _mpViewer->Close();
    if(_mpBackend)
        _mpBackend->Stop();
    if(_mpLoopClosing)
        _mpLoopClosing->Stop();
}


// -------------------------------------------------------------------------------------------------

bool System::RunStep(const cv::Mat &leftImg, const cv::Mat &rightImg, const double &dTimestamp){
    
   bool success =  _mpFrontend->GrabStereoImage(leftImg, rightImg, dTimestamp);
    
    return success;
}


// -------------------------------------------------------------------------
void System::GetCamera(){

    // load the camera params from config file
    float fxLeft = Config::Get<float>("Camera.right.fx");
    float  fyLeft = Config::Get<float>("Camera.right.fy");
    float cxLeft = Config::Get<float>("Camera.right.cx");
    float cyLeft = Config::Get<float>("Camera.right.cy");
    Vec3 tLeft = Vec3::Zero();

    float fxRight = Config::Get<float>("Camera.right.fx");
    float  fyRight = Config::Get<float>("Camera.right.fy");
    float cxRight = Config::Get<float>("Camera.right.cx");
    float  cyRight = Config::Get<float>("Camera.right.cy");
    float bf = Config::Get<float>("Camera.bf");
    float baseline = bf / fxRight;
    Vec3 tRight = Vec3(- baseline, 0, 0);

    cv::Mat distCoefLeft = cv::Mat::zeros(4, 1, CV_32F);
    cv::Mat distCoefRight = cv::Mat::zeros(4, 1, CV_32F);
    if(_mbNeedUndistortion){
        float k1Left = Config::Get<float>("Camera.left.k1");
        float k2Left = Config::Get<float>("Camera.left.k2");
        float p1Left = Config::Get<float>("Camera.left.p1");
        float p2Left = Config::Get<float>("Camera.left.p2");
        distCoefLeft.at<float>(0) = k1Left;
        distCoefLeft.at<float>(1) = k2Left;
        distCoefLeft.at<float>(2) = p1Left;
        distCoefLeft.at<float>(3) = p2Left;

        float k1Right = Config::Get<float>("Camera.right.k1");
        float k2Right = Config::Get<float>("Camera.right.k2");
        float p1Right = Config::Get<float>("Camera.right.p1");
        float p2Right = Config::Get<float>("Camera.right.p2");
        distCoefRight.at<float>(0) = k1Right;
        distCoefRight.at<float>(1) = k2Right;
        distCoefRight.at<float>(2) = p1Right;
        distCoefRight.at<float>(3) = p2Right;
    }

    // set the pose of left camera as identity isometry matrix by default
    _mpCameraLeft = Camera::Ptr(new Camera(fxLeft, fyLeft, cxLeft, cyLeft,
                                        0, SE3(SO3(), tLeft), distCoefLeft));

    _mpCameraRight = Camera::Ptr(new Camera(fxRight, fyRight, cxRight, cyRight,
                                        baseline, SE3(SO3(), tRight), distCoefRight));
}


// ------------------------------------------------------------------------------------------

// the output format is like: 
//      "keyframe id, timestamp, tx, ty, tz, qx, qy, qz, qw" per line
void System::SaveTrajectory(std::string &save_file){
    std::ofstream outfile;
    outfile.open(save_file, std::ios_base::out|std::ios_base::trunc);
    outfile << std::fixed;
    std::map<unsigned long, KeyFrame::Ptr> poses_map;

    for (auto &kf: _mpMap->GetAllKeyFrames()){
        unsigned long keyframe_id = kf.first;
        KeyFrame::Ptr keyframe = kf.second;
        poses_map.insert(make_pair(keyframe_id, keyframe));
    }
    
    for (auto &kf: poses_map){
        unsigned long keyframe_id = kf.first;
        KeyFrame::Ptr keyframe = kf.second;
        double timestamp = keyframe->mdTimeStamp;
        SE3 frame_pose = keyframe->Pose().inverse();
        Vec3 pose_t = frame_pose.translation();
        Mat33 pose_R = frame_pose.rotationMatrix();
        Eigen::Quaterniond pose_q = Eigen::Quaterniond(pose_R);

        outfile << std::setprecision(6) << keyframe_id << " " << timestamp << " "
                << pose_t.transpose() << " " << pose_q.coeffs().transpose() << std::endl;
    }
    outfile.close();
}


// ------------------------------------------------------------------------------------------

// the output format is like: 
//      "Current id, timestamp, tx, ty, tz, qx, qy, qz, qw" 
//      "Loop id, timestamp, tx, ty, tz, qx, qy, qz, qw"
//       two lines as a group
void System::SaveLoopEdges(std::string &save_file){
    std::ofstream outfile;
    outfile.open(save_file, std::ios_base::out|std::ios_base::trunc);
    outfile << std::fixed;
    std::map<unsigned long, KeyFrame::Ptr> poses_map;

    for (auto &kf: _mpMap->GetAllKeyFrames()){
        unsigned long keyframe_id = kf.first;
        KeyFrame::Ptr keyframe = kf.second;
        poses_map.insert(make_pair(keyframe_id, keyframe));
    }
    
    for (auto &kf: poses_map){
        unsigned long keyframe_id = kf.first;
        KeyFrame::Ptr keyframe = kf.second;
        auto loopKF = keyframe->mpLoopKF.lock();
        if(loopKF){
            double timestamp_current = keyframe->mdTimeStamp;
            SE3 frame_pose_current = keyframe->Pose().inverse();
            Vec3 pose_t_current = frame_pose_current.translation();
            Mat33 pose_R_current = frame_pose_current.rotationMatrix();
            Eigen::Quaterniond pose_q_current = Eigen::Quaterniond(pose_R_current);

            outfile << std::setprecision(6) << keyframe_id << " " << timestamp_current << " "
                << pose_t_current.transpose() << " " << pose_q_current.coeffs().transpose() << std::endl;

            double timestamp_loop = loopKF->mdTimeStamp;
            SE3 frame_pose_loop = loopKF->Pose().inverse();
            Vec3 pose_t_loop = frame_pose_loop.translation();
            Mat33 pose_R_loop = frame_pose_loop.rotationMatrix();
            Eigen::Quaterniond pose_q_loop = Eigen::Quaterniond(pose_R_loop);

            outfile << std::setprecision(6) << loopKF->mnKFId << " " << timestamp_loop << " "
                << pose_t_loop.transpose() << " " << pose_q_loop.coeffs().transpose() << std::endl;
        }
    }
    outfile.close();
}

} // namespace myslam

