#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string>
#include <time.h>

//include ros libraries
#include <ros/ros.h>

#include <actionlib/client/simple_action_client.h>
#include <move_base_msgs/MoveBaseAction.h>

#include <geometry_msgs/Twist.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/PoseWithCovarianceStamped.h>
#include <move_base_msgs/MoveBaseGoal.h>
#include <move_base_msgs/MoveBaseActionGoal.h>

#include "sensor_msgs/LaserScan.h"
#include "sensor_msgs/PointCloud2.h"

#include <nav_msgs/Odometry.h>
#include <nav_msgs/OccupancyGrid.h>
#include <nav_msgs/Path.h>
#include <nav_msgs/GetPlan.h>

#include <tf/tf.h>
#include <tf/transform_datatypes.h>
#include <tf/transform_listener.h>

#include <boost/foreach.hpp>
//#define forEach BOOST_FOREACH

/** for global path planner interface */
#include <costmap_2d/costmap_2d_ros.h>
#include <costmap_2d/costmap_2d.h>
#include <nav_core/base_global_planner.h>

#include <geometry_msgs/PoseStamped.h>
#include <angles/angles.h>

//#include <pcl_conversions/pcl_conversions.h>
#include <base_local_planner/world_model.h>
#include <base_local_planner/costmap_model.h>

#include <set>

#include "rrt.h"

using namespace std;
using std::string;

#ifndef RRT_PLANNER
#define RRT_PLANNER

/**
 * @struct cells
 * @brief A struct that represents a cell and its fCost.
 */

#define RANDOM_MAX 2147483647

namespace rrt_plan
{
struct Cell
{
    int posX;
    int posY;
};

class rrt_planner : public nav_core::BaseGlobalPlanner
{
public:

    rrt_planner (ros::NodeHandle &); //this constructor is may be not needed
    rrt_planner ();
    rrt_planner(std::string name, costmap_2d::Costmap2DROS* costmap_ros);

    ros::NodeHandle ROSNodeHandle;

    /** overriden classes from interface nav_core::BaseGlobalPlanner **/
    void initialize(std::string name, costmap_2d::Costmap2DROS* costmap_ros);
    bool makePlan(const geometry_msgs::PoseStamped& start,
                  const geometry_msgs::PoseStamped& goal,
                  std::vector<geometry_msgs::PoseStamped>& plan);

    bool isCellInsideMap(float x, float y);
    void mapToWorld(double mx, double my, double& wx, double& wy);

    bool isStartAndGoalCellsValid(int startCell,int goalCell);
    bool isFree(int CellID); //returns true if the cell is Free
    bool isFree(int i, int j);
    vector <int> findFreeNeighborCell (int CellID);

    vector<int>   findCellPath(RRT &myRRT, int startCell, int goalCell,vector<vector<float> > gridVal);//使用温度场作为启发因子
    void convertToPlan(vector<int> bestPath,std::vector<geometry_msgs::PoseStamped>& plan, geometry_msgs::PoseStamped goal,int goalCell);
    void publishPlan(const std::vector<geometry_msgs::PoseStamped>& path);
    void publishPlan2(const std::vector<geometry_msgs::PoseStamped>& path);
    void getGridVal(vector<vector<float> >& gridVal, int startRowID, int startColID,int goalRowID,int goalColID);
    
    std::vector<geometry_msgs::PoseStamped> middleoptimizationLayer( std::vector<geometry_msgs::PoseStamped> temp_transformed_plan);

    void getIdx(int &x, int &y)
    {
        x = x/mapReduceSize;
        y = y/mapReduceSize;
    }

    void getCorrdinate (float &x, float &y)
    {
        x = x - originX;
        y = y - originY;
    }

    int getCellIndex(int i,int j)//根据cell所处的行和列得到cell的索引值
    {
        return (i*width)+j;
    }

    int getCellRowID(int index)//根据cell的索引值得到行数
    {
        return index/width;
    }

    int getCellColID(int index)//根据索引值得到列数
    {
        return index%width;
    }

    int convertToCellIndex (float x, float y)//根据坐标值得到cell索引值
    {
        int cellIndex;

        float newX = x / resolution;
        float newY = y / resolution;

        cellIndex = getCellIndex(newY, newX);

        return cellIndex;
    }


    void convertToCoordinate(int index, float &x, float &y)//根据cell索引值得到坐标值
    {
        x = getCellColID(index) * resolution;

        y = getCellRowID(index) * resolution;

        x = x + originX;
        y = y + originY;
    }

    bool checkIfOnObstacles(RRT::rrtNode &tempNode);

    void generateTempPoint(RRT::rrtNode &tempNode,int width, int height)//生成一个随机结点
    {
        int x = rand() % width + 1;  //行
        int y = rand() % height + 1;  //列
        tempNode.posX = x;
        tempNode.posY = y;
    }

    void generateRandNodes(RRT &myRRT, vector<RRT::rrtNode> &randNodes,int width, int height)//生成一个随机结点数组
    {
        RRT::rrtNode loopNode;

        for(int i=0; i<randNodeNum; i++)
        {
            int x = rand() % width + 1;  //行
            int y = rand() % height + 1;  //列
          
            loopNode.posX = x;
            loopNode.posY = y;

            randNodes.push_back(loopNode);
        }

    }

    /*
      * 求两个向量之间的旋转角
      * 首先明确几个数学概念：
      * 1. 极轴沿逆时针转动的方向是正方向
      * 2. 两个向量之间的夹角theta， 是指(A^B)/(|A|*|B|) = cos(theta)，0<=theta<=180 度， 而且没有方向之分
      * 3. 两个向量的旋转角，是指从向量p1开始，逆时针旋转，转到向量p2时，所转过的角度， 范围是 0 ~ 360度
      * 计算向量p1到p2的旋转角，算法如下：
      * 首先通过点乘和arccosine的得到两个向量之间的夹角
      * 然后判断通过差乘来判断两个向量之间的位置关系
      * 如果p2在p1的顺时针方向, 返回arccose的角度值, 范围0 ~ 180.0(根据右手定理,可以构成正的面积)
      * 否则返回 360.0 - arecose的值, 返回180到360(根据右手定理,面积为负)
      */
    double getRotateAngle(double x1, double y1, double x2, double y2)
    {
        const double epsilon = 1.0e-6;
        const double nyPI = acos(-1.0);
        double dist, dot, degree, angle;

        // normalize
        dist = sqrt( x1 * x1 + y1 * y1 );
        x1 /= dist;
        y1 /= dist;
        dist = sqrt( x2 * x2 + y2 * y2 );
        x2 /= dist;
        y2 /= dist;
        // dot product
        dot = x1 * x2 + y1 * y2;
        if ( fabs(dot-1.0) <= epsilon )
            angle = 0.0;
        else if ( fabs(dot+1.0) <= epsilon )
            angle = nyPI;
        else
        {
            double cross;

            angle = acos(dot);
            //cross product
            cross = x1 * y2 - x2 * y1;
            // vector p2 is clockwise from vector p1
            // with respect to the origin (0.0)
            if (cross < 0 )
            {
                angle = 2 * nyPI - angle;
            }
        }
        //degree = angle *  180.0 / nyPI;
        //return degree;
        return angle;
    }

    bool addNewNodetoRRT(RRT &myRRT, vector<RRT::rrtNode> &randNodes,  float goalX, float goalY , int rrtStepSize,vector<vector<float> > gridVal)  //使用温度场作为启发因子
    {
        vector<RRT::rrtNode> nearestNodes;  //nearest结点数组
        vector<RRT::rrtNode> tempNodes;  //临时结点数组
        RRT::rrtNode loopNode;

        for(int i=0; i<randNodeNum; i++)
        {
            int tempNearestNodeID = myRRT.getNearestNodeID(randNodes[i].posX,randNodes[i].posY);
            RRT::rrtNode tempNearestNode=myRRT.getNode(tempNearestNodeID);
            nearestNodes.push_back(tempNearestNode);
        }

        for(int i=0; i<randNodeNum; i++)
        {
            double theta = atan2(randNodes[i].posY - nearestNodes[i].posY, randNodes[i].posX - nearestNodes[i].posX);//两个点形成的斜率
            loopNode.posX = (int)(nearestNodes[i].posX + (rrtStepSize * cos(theta)));
            loopNode.posY = (int)(nearestNodes[i].posY + (rrtStepSize * sin(theta)));
            loopNode.parentID=nearestNodes[i].nodeID;  //temp结点的父结点是它的nearest结点

            tempNodes.push_back(loopNode);
        }

        vector<float> evaluateCost;  //启发函数估价

        for(int i=0; i<tempNodes.size(); i++)
        {
            int tempIndex=i;
            RRT::rrtNode tempNode=tempNodes[i];

            if(checkIfOnObstacles(tempNode))//先检查temp结点，不在障碍物上
            {
                //float costG=nearestNodes[i].depth*rrtStepSize;
                float costH=gridVal[tempNode.posX/mapReduceSize][tempNode.posY/mapReduceSize];  //温度场作为代价值
                evaluateCost.push_back(costH);
            }
            else  //tempNode结点在障碍物上
            {
                evaluateCost.push_back(infinity);  //障碍物上代价无穷大
            }
        }

        int tempIndex=-1;
        float tempCost=infinity;
        for(int i=0; i<evaluateCost.size(); i++)
        {
            if(evaluateCost[i]<tempCost)
            {
                tempIndex=i;
                tempCost=evaluateCost[i];
            }
        }

        if(tempIndex!=-1)
        {
            RRT::rrtNode successNode=tempNodes[tempIndex];
            RRT::rrtNode nearestNode=nearestNodes[tempIndex];
            float costN=gridVal[nearestNode.posX/mapReduceSize][nearestNode.posY/mapReduceSize];  //温度场作为代价值
            
            if(evaluateCost[tempIndex]>costN)  //防止RRT随机树退化
                return false;
            
            float tempToParentDistance=rrtStepSize;
            bool regressionFlag=true;
            for(int i=0; i<myRRT.getTreeSize(); i++)
            {
                float tempDistance=sqrt(pow(myRRT.getNode(i).posX - successNode.posX,2) + pow(myRRT.getNode(i).posY - successNode.posY,2));

                if(tempToParentDistance-tempDistance>1)
                {
                    regressionFlag=false;
                    break;
                }
            }
            
            if(regressionFlag)
            {
                successNode.nodeID = myRRT.getTreeSize();//设置tempNode的id值，结点个数代表编号
                successNode.depth=nearestNodes[tempIndex].depth+1;//当前结点深度等于其父结点深度加1

                myRRT.addNewNode(successNode);

                return true;
            }
        }

        return false;
    }

    bool checkNodetoGoal(int X, int Y, RRT::rrtNode &tempNode)
    {
        double distance = sqrt(pow(X-tempNode.posX,2)+pow(Y-tempNode.posY,2));
        if(distance < rrtStepSize)  //goal与nearest之间距离小于rrtStepSize时即认为到达目标点
        {
            return true;
        }
        return false;
    }

    float originX;
    float originY;
    float resolution;
    costmap_2d::Costmap2DROS* costmap_ros_;
    double step_size_, min_dist_from_robot_;
    costmap_2d::Costmap2D* costmap_;
    //base_local_planner::WorldModel* world_model_; ///< @brief The world model that the controller will use
    bool initialized_;
    int width;
    int height;

    ros::Publisher plan_pub_;
    ros::Publisher plan_pub_2;
};

};
#endif
