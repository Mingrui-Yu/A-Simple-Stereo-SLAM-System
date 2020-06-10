# A Simple Stereo SLAM System

[[GitHub](https://github.com/Mingrui-Yu/A-Simple-Stereo-SLAM-System)]

This is a simple stereo SLAM system with a deep-learning based loop closure module ([Seq-CALC](https://github.com/Mingrui-Yu/Seq-CALC)). As a beginner of SLAM, I made this system mainly in order to practice my coding and engineering skills to build a full SLAM system by myself. 

I chose to build this system based on stereo cameras because it is easiler, without complicated work on initialization or dealing with the unknown scale. The structure of the system is simple and clear, in which I didn't apply much detailed optimization. Thus, the performance of this system is not outstanding. However, I hold the view that such a simple structure may be friendly for a SLAM beginner to study the body frame of a full SLAM system. It will be definitely a tough work for a beginner to study, for example,  ORB-SLAM2, a complex system with more than 10 thousand lines of code and a lot of tricks to improve its performance. 

It is truly a pleasure for me if this project can help you. 


# Method

The system contains three thread:
* Frontend thread
* Backend thread
* LoopClosing thread

In Frontend, it will track the motion based on feature points and LK flow. If the number of tracked keypoints is lower than a thresold, it will detect new features and create a keyframe. Mappoints are created by triangulating the matched feature points in left/right images.

In Backend, it will maintain a global map and an local active map. The active map is like a sliding window, containing a fixed number of keyframes and observed mappoints. Optimization of the active map is done in Backend.

In LoopClosing, it will first try to detect a Candidate Loop KF of the Current KF using DeepLCD. If succeed, it will then match the keypoints in Candidate KF and Current KF, which is used to compute the correct pose of Current KF using PnP and g2o optimization. If the number of inliers is higher than a threshold, the loop detection will be considered as a success, and loop correction is applyed: first, it will correct the keyframe poses and mappoint positions in active map; second, a pose graph optimization of the global map will be applied.

# Performance


