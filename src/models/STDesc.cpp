//
// Created by hr on 23-4-14.
//
#include "../include/std_detector/STDesc.h"
#include <execution>

/* 累积点云，增加点云密度 */
void down_sampling_voxel(pcl::PointCloud<pcl::PointXYZI> &pl_feat, double voxel_size) {
    if (voxel_size < 0.01)
        return;
    // struct M_POINT: float xyz[3]; float intensity; int count = 0;
    std::unordered_map<VOXEL_LOC, M_POINT> voxel_map;
    uint plsize = pl_feat.size();
    for (uint i = 0; i < plsize; ++i) {
        pcl::PointXYZI &p_c = pl_feat[i];
        float loc_xyz[3];
        for (int j = 0; j < 3; ++j) {
            loc_xyz[j] = static_cast<float>(p_c.data[j] / voxel_size);
            if (loc_xyz[j] < 0)
                loc_xyz[j] -= 1.0;
        }
        VOXEL_LOC position((int64_t) loc_xyz[0], (int64_t) loc_xyz[1], (int64_t) loc_xyz[2]);
        auto iter = voxel_map.find(position);
        if (iter != voxel_map.end()) {
            /* 累积点云， 增加点云密度 */
            iter->second.xyz[0] += p_c.x;
            iter->second.xyz[1] += p_c.y;
            iter->second.xyz[2] += p_c.z;
            iter->second.intensity += p_c.intensity;
            iter->second.count++;
        } else {
            M_POINT anp;
            anp.xyz[0] = p_c.x;
            anp.xyz[1] = p_c.y;
            anp.xyz[2] = p_c.z;
            anp.intensity = p_c.intensity;
            anp.count = 1;
            voxel_map[position] = anp;
        }
    }
    plsize = voxel_map.size();
    pl_feat.clear();
    pl_feat.resize(plsize);

    uint i = 0;
    for (auto &iter: voxel_map) {
        pl_feat[i].x = iter.second.xyz[0] / static_cast<float>(iter.second.count);
        pl_feat[i].y = iter.second.xyz[1] / static_cast<float>(iter.second.count);
        pl_feat[i].z = iter.second.xyz[2] / static_cast<float>(iter.second.count);
        pl_feat[i].intensity = iter.second.intensity / static_cast<float>(iter.second.count);
        i++;
    }
}

/* 参数读取 */
void read_parameters(ros::NodeHandle &nh, ConfigSetting &config_setting) {
    // pre-preocess
    nh.param<double>("ds_size", config_setting.ds_size_, 0.5);
    nh.param<int>("maximum_corner_num", config_setting.maximum_corner_num_, 100);
    // key points
    nh.param<double>("plane_merge_normal_thre", config_setting.plane_merge_normal_thre_, 0.1);
    nh.param<double>("plane_detection_thre", config_setting.plane_detection_thre_, 0.01);
    nh.param<double>("voxel_size", config_setting.voxel_size_, 2.0);
    nh.param<int>("voxel_init_num", config_setting.voxel_init_num_, 10);
    nh.param<double>("proj_image_resolution", config_setting.proj_image_resolution_, 0.5);
    nh.param<double>("proj_dis_min", config_setting.proj_dis_min_, 0);
    nh.param<double>("proj_dis_max", config_setting.proj_dis_max_, 2);
    nh.param<double>("corner_thre", config_setting.corner_thre_, 10);
    // std descriptor
    nh.param<int>("descriptor_near_num", config_setting.descriptor_near_num_, 10);
    nh.param<double>("descriptor_min_len", config_setting.descriptor_min_len_, 2);
    nh.param<double>("descriptor_max_len", config_setting.descriptor_max_len_, 50);
    nh.param<double>("non_max_suppression_radius", config_setting.non_max_suppression_radius_, 2.0);
    nh.param<double>("std_side_resolution", config_setting.std_side_resolution_, 0.2);
    // candidate search
    nh.param<int>("skip_near_num", config_setting.skip_near_num_, 50);
    nh.param<int>("candidate_num", config_setting.candidate_num_, 50);
    nh.param<int>("sub_frame_num", config_setting.sub_frame_num_, 10);
    nh.param<double>("rough_dis_threshold", config_setting.rough_dis_threshold_, 0.01);
    nh.param<double>("vertex_diff_threshold", config_setting.vertex_diff_threshold_, 0.5);
    nh.param<double>("icp_threshold", config_setting.icp_threshold_, 0.5);
    nh.param<double>("normal_threshold", config_setting.normal_threshold_, 0.2);
    nh.param<double>("dis_threshold", config_setting.dis_threshold_, 0.5);

    nh.param<double>("historyKeyframeSearchTimeDiff", config_setting.historyKeyframeSearchTimeDiff, 10.0);

    std::cout << "Successfully load parameters:" << std::endl;
    std::cout << "----------------Main Parameters-------------------" << std::endl;
    std::cout << "voxel size:" << config_setting.voxel_size_ << std::endl;
    std::cout << "loop detection threshold: " << config_setting.icp_threshold_ << std::endl;
    std::cout << "sub-frame number: " << config_setting.sub_frame_num_ << std::endl;
    std::cout << "candidate number: " << config_setting.candidate_num_ << std::endl;
    std::cout << "maximum corners size: " << config_setting.maximum_corner_num_ << std::endl;
}

/* 从文件中加载位姿与时间戳信息 */
void load_pose_with_time(const std::string &pose_file,
                         std::vector<std::pair<Eigen::Vector3d, Eigen::Matrix3d>> &poses_vec,
                         std::vector<double> &times_vec) {}

/* 时间计算函数 */
double time_inc(std::chrono::_V2::system_clock::time_point &t_end,
                std::chrono::_V2::system_clock::time_point &t_begin) {
    return std::chrono::duration_cast<std::chrono::duration<double>>(t_end - t_begin).count() * 1000;
}

/* vec -> point */
pcl::PointXYZI vec2point(const Eigen::Vector3d &vec) {
    pcl::PointXYZI pi;
    pi.x = static_cast<float>(vec[0]);
    pi.y = static_cast<float>(vec[1]);
    pi.z = static_cast<float>(vec[2]);
    return pi;
}

/* point -> vector */
Eigen::Vector3d point2vec(const pcl::PointXYZI &pi) {
    return {pi.x, pi.y, pi.z};
}

/* 自定义比较函数 double:intensity */
bool attach_greater_sort(std::pair<double, int> a, std::pair<double, int> b) {
    return (a.first < b.first);
}

/* 配对描述子发布函数 */
void publish_std_pairs(const std::vector<std::pair<STDesc, STDesc>> &match_std_pairs,
                       const ros::Publisher &std_publisher) {
    visualization_msgs::MarkerArray ma_line;
    visualization_msgs::Marker m_line;
    m_line.type = visualization_msgs::Marker::LINE_LIST;
    m_line.action = visualization_msgs::Marker::ADD;
    m_line.ns = "lines";
    // Don't forget to set the alpha!
    m_line.scale.x = 0.25;
    m_line.pose.orientation.w = 1.0;
    m_line.header.frame_id = "camera_init";
    m_line.id = 0;
    int max_pub_cnt = 1;
    for (auto var: match_std_pairs) {
        if (max_pub_cnt > 100)
            break;
        max_pub_cnt++;
        m_line.color.a = 0.8;
        m_line.points.clear();
        m_line.color.r = 138.0 / 255;
        m_line.color.g = 226.0 / 255;
        m_line.color.b = 52.0 / 255;
        geometry_msgs::Point p;
        p.x = var.second.vertex_A_[0];
        p.y = var.second.vertex_A_[1];
        p.z = var.second.vertex_A_[2];
        Eigen::Vector3d t_p;
        t_p << p.x, p.y, p.z;
        p.x = t_p[0];
        p.y = t_p[1];
        p.z = t_p[2];
        m_line.points.push_back(p);
        p.x = var.second.vertex_B_[0];
        p.y = var.second.vertex_B_[1];
        p.z = var.second.vertex_B_[2];
        t_p << p.x, p.y, p.z;
        p.x = t_p[0];
        p.y = t_p[1];
        p.z = t_p[2];
        m_line.points.push_back(p);
        ma_line.markers.push_back(m_line); // 边AB
        m_line.id++;
        m_line.points.clear();
        p.x = var.second.vertex_C_[0];
        p.y = var.second.vertex_C_[1];
        p.z = var.second.vertex_C_[2];
        t_p << p.x, p.y, p.z;
        p.x = t_p[0];
        p.y = t_p[1];
        p.z = t_p[2];
        m_line.points.push_back(p);
        p.x = var.second.vertex_B_[0];
        p.y = var.second.vertex_B_[1];
        p.z = var.second.vertex_B_[2];
        t_p << p.x, p.y, p.z;
        p.x = t_p[0];
        p.y = t_p[1];
        p.z = t_p[2];
        m_line.points.push_back(p);
        ma_line.markers.push_back(m_line); // 边BC
        m_line.id++;
        m_line.points.clear();
        p.x = var.second.vertex_C_[0];
        p.y = var.second.vertex_C_[1];
        p.z = var.second.vertex_C_[2];
        t_p << p.x, p.y, p.z;
        p.x = t_p[0];
        p.y = t_p[1];
        p.z = t_p[2];
        m_line.points.push_back(p);
        p.x = var.second.vertex_A_[0];
        p.y = var.second.vertex_A_[1];
        p.z = var.second.vertex_A_[2];
        t_p << p.x, p.y, p.z;
        p.x = t_p[0];
        p.y = t_p[1];
        p.z = t_p[2];
        m_line.points.push_back(p);
        ma_line.markers.push_back(m_line); // 边AC
        m_line.id++;
        m_line.points.clear();
        /* 另一个描述子 */
        m_line.points.clear();
        m_line.color.r = 1;
        m_line.color.g = 1;
        m_line.color.b = 1;
        p.x = var.first.vertex_A_[0];
        p.y = var.first.vertex_A_[1];
        p.z = var.first.vertex_A_[2];
        m_line.points.push_back(p);
        p.x = var.first.vertex_B_[0];
        p.y = var.first.vertex_B_[1];
        p.z = var.first.vertex_B_[2];
        m_line.points.push_back(p);
        ma_line.markers.push_back(m_line);
        m_line.id++;
        m_line.points.clear();
        p.x = var.first.vertex_C_[0];
        p.y = var.first.vertex_C_[1];
        p.z = var.first.vertex_C_[2];
        m_line.points.push_back(p);
        p.x = var.first.vertex_B_[0];
        p.y = var.first.vertex_B_[1];
        p.z = var.first.vertex_B_[2];
        m_line.points.push_back(p);
        ma_line.markers.push_back(m_line);
        m_line.id++;
        m_line.points.clear();
        p.x = var.first.vertex_C_[0];
        p.y = var.first.vertex_C_[1];
        p.z = var.first.vertex_C_[2];
        m_line.points.push_back(p);
        p.x = var.first.vertex_A_[0];
        p.y = var.first.vertex_A_[1];
        p.z = var.first.vertex_A_[2];
        m_line.points.push_back(p);
        ma_line.markers.push_back(m_line);
        m_line.id++;
        m_line.points.clear();
    }
    for (int i = 0; i < 100 * 6; ++i) {
        m_line.color.a = 0.00; // 显示完就把线设置为透明（删除操作）
        ma_line.markers.push_back(m_line);
        m_line.id++;
    }
    std_publisher.publish(ma_line);
    m_line.id = 0;
    ma_line.markers.clear();
}

/* 构建STD描述子 */
void STDescManager::GenerateSTDescs(pcl::PointCloud<pcl::PointXYZI>::Ptr &input_cloud, std::vector<STDesc> &stds_vec) {
    /* step1, voxelization and plane dection */
    std::unordered_map<VOXEL_LOC, OctoTree *> voxel_map;
    init_voxel_map(input_cloud, voxel_map);
    pcl::PointCloud<pcl::PointXYZINormal>::Ptr plane_cloud(new pcl::PointCloud<pcl::PointXYZINormal>);
    getPlane(voxel_map, plane_cloud);
    printf("[Description] planes size: %lu\n", plane_cloud->size());
    plane_cloud_vec_.push_back(plane_cloud);
    /* step2, build connection for planes in the voxel map */
    build_connection(voxel_map);
    /* step3, extraction corner points */
    pcl::PointCloud<pcl::PointXYZINormal>::Ptr corner_points(new pcl::PointCloud<pcl::PointXYZINormal>);
    corner_extractor(voxel_map, input_cloud, corner_points);
    corner_cloud_vec_.push_back(corner_points);
    printf("[Description] corners size: %lu\n", corner_points->size());
    /* step4, generate stable triangle descriptors */
    stds_vec.clear();
    build_stdesc(corner_points, stds_vec);
    printf("[Description] stds size: %lu\n", stds_vec.size());
    /* step5, clear memory */
    // TODO：每次都重新构建体素，可以使用之前建立好的体素八叉树吗？会不会过紧？
    for (auto &iter: voxel_map) {
        delete (iter.second);
    }
}

/* 初始化体素空间 */
// TODO 是否需要将此函数声明为const
void STDescManager::init_voxel_map(const pcl::PointCloud<pcl::PointXYZI>::Ptr &input_cloud,
                                   std::unordered_map<VOXEL_LOC, OctoTree *> &voxel_map) const {
    /* 1.遍历每个input_cloud，构建八叉树并将点云通过哈希表存入其中 */
    uint plsize = input_cloud->size();
    for (uint i = 0; i < plsize; ++i) {
        Eigen::Vector3d p_c(input_cloud->points[i].x, input_cloud->points[i].y, input_cloud->points[i].z);
        float loc_xyz[3];
        for (int j = 0; j < 3; ++j) {
            loc_xyz[j] = static_cast<float>(p_c[j] / config_setting_.voxel_size_);
            if (loc_xyz[j] < 0)
                loc_xyz[j] -= 1.0;
        }
        VOXEL_LOC position((int64_t) loc_xyz[0], (int64_t) loc_xyz[1], (int64_t) loc_xyz[2]);
        auto iter = voxel_map.find(position);
        if (iter != voxel_map.end()) {
            voxel_map[position]->voxel_points_.push_back(p_c);
        } else {
            auto *octo_tree = new OctoTree(config_setting_);
            voxel_map[position] = octo_tree;
            voxel_map[position]->voxel_points_.push_back(p_c);
        }
    }
    std::vector<std::unordered_map<VOXEL_LOC, OctoTree *>::iterator> iter_list;
    std::vector<size_t> index;
    size_t i = 0;
    for (auto iter = voxel_map.begin(); iter != voxel_map.end(); ++iter) {
        index.push_back(i);
        i++;
        iter_list.push_back(iter);
    }
    /* 2.对每个体素的八叉树进行初始化，即当voxel_points_.size() > voxel_init_num时，初始化平面 */
    // TODO:对下面过程进行加速
//#ifdef MP_EN
//    omp_set_num_threads(MP_PROC_NUM);
//    std::cout << "omp num:" << MP_PROC_NUM << std::endl;
//#pragma omp parallel for
//#endif
    for (int j = 0; j < index.size(); ++j) {
        iter_list[j]->second->init_octo_tree();
    }
//    std::cout << "voxel num:" << index.size() << std::endl;
//    std::for_each(std::execution::par_unseq, index.begin(), index.end(),
//                  [&](const size_t &i) { iter_list[i]->second->init_octo_tree(); })
}

/* 平面增长 */
// TODO 存在warning：Index may be out of bounds，（但是源代码中没有出现warning）
void STDescManager::build_connection(std::unordered_map<VOXEL_LOC, OctoTree *> &voxel_map) const {
    for (auto iter = voxel_map.begin(); iter != voxel_map.end(); ++iter) {
        if (iter->second->plane_ptr_->is_plane_) {
            OctoTree *current_octo = iter->second;
            for (int i = 0; i < 6; ++i) {
                VOXEL_LOC neighbor = iter->first;
                if (i == 0) {
                    neighbor.x = neighbor.x + 1;
                } else if (i == 1) {
                    neighbor.y = neighbor.y + 1;
                } else if (i == 2) {
                    neighbor.z = neighbor.z + 1;
                } else if (i == 3) {
                    neighbor.x = neighbor.x - 1;
                } else if (i == 4) {
                    neighbor.y = neighbor.y - 1;
                } else if (i == 5) {
                    neighbor.z = neighbor.z - 1;
                }
                auto near = voxel_map.find(neighbor);
                if (near == voxel_map.end()) {
                    current_octo->is_check_connect_[i] = true;
                    current_octo->connect_[i] = false;
                } else {
                    if (!current_octo->is_check_connect_[i]) {
                        OctoTree *near_octo = near->second;
                        current_octo->is_check_connect_[i] = true;
                        // near_octo的索引（和i相反）
                        int j;
                        if (i >= 3) {
                            j = i - 3; // j = 0,1,2
                        } else {
                            j = i + 3; // j = 3,4,5
                        }
                        near_octo->is_check_connect_[j] = true;
                        if (near_octo->plane_ptr_->is_plane_) {
                            Eigen::Vector3d normal_diff =
                                    current_octo->plane_ptr_->normal_ - near_octo->plane_ptr_->normal_;
                            Eigen::Vector3d normal_add =
                                    current_octo->plane_ptr_->normal_ + near_octo->plane_ptr_->normal_;
                            /* 合并平面 */
                            if (normal_diff.norm() < config_setting_.plane_merge_normal_thre_ ||
                                normal_add.norm() < config_setting_.plane_merge_normal_thre_) {
                                current_octo->connect_[i] = true;
                                near_octo->connect_[j] = true;
                                current_octo->connect_tree_[i] = near_octo;
                                near_octo->connect_tree_[j] = current_octo;
                            } else {
                                current_octo->connect_[i] = false;
                                near_octo->connect_[j] = false;
                            }
                        } else {
                            /* 边界体素 */
                            current_octo->connect_[i] = false;
                            near_octo->connect_[j] = true;
                            near_octo->connect_tree_[j] = current_octo;
                        }
                    }
                }
            }
        }
    }
}

/* 从voxel_map中获得合法平面 */
void STDescManager::getPlane(const std::unordered_map<VOXEL_LOC, OctoTree *> &voxel_map,
                             pcl::PointCloud<pcl::PointXYZINormal>::Ptr &plane_cloud) {
    for (const auto &iter: voxel_map) {
        if (iter.second->plane_ptr_->is_plane_) {
            pcl::PointXYZINormal pi;
            pi.x = iter.second->plane_ptr_->center_[0];
            pi.y = iter.second->plane_ptr_->center_[1];
            pi.z = iter.second->plane_ptr_->center_[2];
            pi.normal_x = iter.second->plane_ptr_->normal_[0];
            pi.normal_y = iter.second->plane_ptr_->normal_[1];
            pi.normal_z = iter.second->plane_ptr_->normal_[2];
            plane_cloud->push_back(pi);
        }
    }
}

/* 从边界体素中提取特征点 */
void STDescManager::corner_extractor(std::unordered_map<VOXEL_LOC, OctoTree *> &voxel_map,
                                     const pcl::PointCloud<pcl::PointXYZI>::Ptr &input_cloud,
                                     pcl::PointCloud<pcl::PointXYZINormal>::Ptr &corner_points) {
    pcl::PointCloud<pcl::PointXYZINormal>::Ptr prepare_corner_points(new pcl::PointCloud<pcl::PointXYZINormal>);
    // Avoid inconsistent voxel cutting caused by different view point
    // 避免因视角不同而导致体素切割不一致
    /* 体素周围元素（-1, 0, 1） */
    std::vector<Eigen::Vector3i> voxel_round;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            for (int z = -1; z <= 1; z++) {
                Eigen::Vector3i voxel_inc(x, y, z);
                voxel_round.push_back(voxel_inc);
            }
        }
    }
    /* 确定要投影的点并完成投影 */
    for (auto iter = voxel_map.begin(); iter != voxel_map.end(); iter++) {
        /* 对边界点操作 */
        if (!iter->second->plane_ptr_->is_plane_) {
            VOXEL_LOC current_position = iter->first;
            OctoTree *current_octo = iter->second;
            int connect_index = -1;
            for (int i = 0; i < 6; ++i) {
                /* 有对应平面再投影 */
                if (current_octo->connect_[i]) {
                    connect_index = i;
                    OctoTree *connect_octo = current_octo->connect_tree_[connect_index];
                    bool use = false;
                    for (int j = 0; j < 6; ++j) {
                        // TODO:进行过平面增长且合并了至少一个平面
                        if (connect_octo->is_check_connect_[j] && connect_octo->connect_[j]) {
                            use = true;
                        }
                    }
                    // if no plane near the voxel, skip
                    if (!use) continue;
                    // 只投影点数量大于10的体素
                    if (current_octo->voxel_points_.size() > 10) {
                        // 要投影到的平面的法向量和中点
                        Eigen::Vector3d projection_normal =
                                current_octo->connect_tree_[connect_index]->plane_ptr_->normal_;
                        Eigen::Vector3d projection_center =
                                current_octo->connect_tree_[connect_index]->plane_ptr_->center_;
                        std::vector<Eigen::Vector3d> proj_points;
                        // proj the boundary voxel and nearby voxel onto adjacent plane
                        // 将边界体素和附近边界体素投影到相邻的平面上
                        for (auto voxel_inc: voxel_round) {
                            VOXEL_LOC connect_project_position = current_position;
                            connect_project_position.x += voxel_inc[0];
                            connect_project_position.y += voxel_inc[1];
                            connect_project_position.z += voxel_inc[2];
                            auto iter_near = voxel_map.find(connect_project_position);
                            if (iter_near != voxel_map.end()) {
                                bool skip_flag = false;
                                if (!voxel_map[connect_project_position]->plane_ptr_->is_plane_) {
                                    // 判断是否出现重复投影
                                    if (voxel_map[connect_project_position]->is_project_) {
                                        for (auto &normal: voxel_map[connect_project_position]->proj_normal_vec_) {
                                            Eigen::Vector3d normal_diff = projection_normal - normal;
                                            Eigen::Vector3d normal_add = projection_normal + normal;
                                            // check if repeated project
                                            if (normal_diff.norm() < 0.5 || normal_add.norm() < 0.5) {
                                                skip_flag = true;
                                            }
                                        }
                                    }
                                    if (skip_flag) continue;
                                    for (size_t j = 0;
                                         j < voxel_map[connect_project_position]->voxel_points_.size(); ++j) {
                                        proj_points.push_back(voxel_map[connect_project_position]->voxel_points_[j]);
                                        voxel_map[connect_project_position]->is_project_ = true;
                                        voxel_map[connect_project_position]->proj_normal_vec_.push_back(
                                                projection_normal);
                                    }
                                }
                            }
                        }
                        // here do the 2D projection and corner extraction
                        pcl::PointCloud<pcl::PointXYZINormal>::Ptr sub_corner_points(
                                new pcl::PointCloud<pcl::PointXYZINormal>);
                        extract_corner(projection_center, projection_normal, proj_points, sub_corner_points);
                        for (auto pi: sub_corner_points->points) {
                            prepare_corner_points->push_back(pi);
                        }
                    }
                }
            }
        }
    }
    // 去除一些不好的点
    non_maxi_suppression(prepare_corner_points);

    /* 如果提取的corner个数过多，则只保留梯度更大的corner */
    if (config_setting_.maximum_corner_num_ > prepare_corner_points->size()) {
        corner_points = prepare_corner_points;
    } else {
        std::vector<std::pair<double, int>> attach_vec;
        for (size_t i = 0; i < prepare_corner_points->size(); i++) {
            // TODO attach_vec.push_back(std::pair<double, int>(prepare_corner_points->points[i].intensity, i));
            attach_vec.emplace_back(prepare_corner_points->points[i].intensity, i);
        }
        std::sort(attach_vec.begin(), attach_vec.end(), attach_greater_sort);
        for (size_t i = 0; i < config_setting_.maximum_corner_num_; i++) {
            corner_points->points.push_back(prepare_corner_points->points[attach_vec[i].second]);
        }
    }
}

/* 2D投影以及corner提取 */
void STDescManager::extract_corner(const Eigen::Vector3d &proj_center, const Eigen::Vector3d &proj_normal,
                                   const std::vector<Eigen::Vector3d> &proj_points,
                                   pcl::PointCloud<pcl::PointXYZINormal>::Ptr &corner_points) const {
    double resolution = config_setting_.proj_image_resolution_;
    double dis_threshold_min = config_setting_.proj_dis_min_;
    double dis_threshold_max = config_setting_.proj_dis_max_;
    // ABCD对应平面方程一般式中的系数 Ax+By+Cz+D = 0
    double A = proj_normal[0];
    double B = proj_normal[1];
    double C = proj_normal[2];
    double D = -(A * proj_center[0] + B * proj_center[1] + C * proj_center[2]); // 也是求点面距离第二项
    /* 求解平面基向量 */
    Eigen::Vector3d x_axis(1, 1, 0);  // 与(A,B,C)垂直 使得与x轴和y轴的夹角相等
    if (C != 0) {
        x_axis[2] = -(A + B) / C; // -(A + B) / C 为基向量x_axis在z轴上的投影
    } else if (B != 0) {
        x_axis[1] = -A / B; // 基向量x_axis在y轴上的投影
    } else {
        x_axis[0] = 0; // 设为(0,1,0) 使得 x_axis 与法向量垂直且与 x 轴的夹角最小
        x_axis[1] = 1;
    }
    x_axis.normalize(); // x方向基向量
    Eigen::Vector3d y_axis = proj_normal.cross(x_axis);
    y_axis.normalize(); // y方向基向量
    double ax = x_axis[0];
    double bx = x_axis[1];
    double cx = x_axis[2];
    double dx = -(ax * proj_center[0] + bx * proj_center[1] + cx * proj_center[2]);
    double ay = y_axis[0];
    double by = y_axis[1];
    double cy = y_axis[2];
    double dy = -(ay * proj_center[0] + by * proj_center[1] + cy * proj_center[2]);
    std::vector<Eigen::Vector2d> point_list_2d;
    /* 得到投影后的点的二维坐标 */
    for (const auto &proj_point: proj_points) {
        double x = proj_point[0];
        double y = proj_point[1];
        double z = proj_point[2];
        double dis = fabs(x * A + y * B + z * C + D); // 点面距离 d = n * p - n * q
        if (dis < dis_threshold_min || dis > dis_threshold_max) {
            continue;
        }
        // 投影到平面后的三维坐标
        Eigen::Vector3d cur_project;
        cur_project[0] = (-A * (B * y + C * z + D) + x * (B * B + C * C)) / (A * A + B * B + C * C);
        cur_project[1] = (-B * (A * x + C * z + D) + y * (A * A + C * C)) / (A * A + B * B + C * C);
        cur_project[2] = (-C * (A * x + B * y + D) + z * (A * A + B * B)) / (A * A + B * B + C * C);
        pcl::PointXYZ p;
        p.x = cur_project[0];
        p.y = cur_project[1];
        p.z = cur_project[2];

        // 投影到二维坐标系
        double project_x = cur_project[0] * ay + cur_project[1] * by + cur_project[2] * cy + dy; // 点面距离
        double project_y = cur_project[0] * ax + cur_project[1] * bx + cur_project[2] * cx + dx;
        Eigen::Vector2d p_2d(project_x, project_y);
        point_list_2d.push_back(p_2d);
    }
    double min_x = 10;
    double max_x = -10;
    double min_y = 10;
    double max_y = -10;
    if (point_list_2d.size() <= 5) {
        return;
    }
    for (auto pi: point_list_2d) {
        if (pi[0] < min_x) {
            min_x = pi[0];
        }
        if (pi[0] > max_x) {
            max_x = pi[0];
        }
        if (pi[1] < min_y) {
            min_y = pi[1];
        }
        if (pi[1] > max_y) {
            max_y = pi[1];
        }
    }
    /* segment project cloud with a fixed resolution */
    int segment_base_num = 5;
    double segment_len = segment_base_num * resolution; // 5 * 0.25
    int x_segment_num = static_cast<int>((max_x - min_x) / segment_len + 1);
    int y_segment_num = static_cast<int>((max_y - min_y) / segment_len + 1);
    int x_axis_len = static_cast<int>((max_x - min_x) / resolution + segment_base_num);
    int y_axis_len = static_cast<int>((max_y - min_y) / resolution + segment_base_num);
    std::vector<Eigen::Vector2d> img_container[x_axis_len][y_axis_len];
    // 初始化给定分辨率图像
    // TODO 选择更优的初始化动态数组方式
    double img_count_array[x_axis_len][y_axis_len];
    double gradient_array[x_axis_len][y_axis_len];
    double mean_x_array[x_axis_len][y_axis_len];
    double mean_y_array[x_axis_len][y_axis_len];
    memset(img_count_array, 0, sizeof img_count_array);
    memset(gradient_array, 0, sizeof gradient_array);
    memset(mean_x_array, 0, sizeof mean_x_array);
    memset(mean_y_array, 0, sizeof mean_y_array);
    for (int x = 0; x < x_axis_len; x++) {
        for (int y = 0; y < y_axis_len; y++) {
            img_count_array[x][y] = 0;
            mean_x_array[x][y] = 0;
            mean_y_array[x][y] = 0;
            gradient_array[x][y] = 0;
            std::vector<Eigen::Vector2d> single_container;
            img_container[x][y] = single_container;
        }
    }
    /* 将每个投影到点保存到给定分辨率的图像中 */
    for (auto point_2d: point_list_2d) {
        int x_index = static_cast<int>((point_2d[0] - min_x) / resolution);
        int y_index = static_cast<int>((point_2d[1] - min_y) / resolution);
        mean_x_array[x_index][y_index] += point_2d[0];
        mean_y_array[x_index][y_index] += point_2d[1];
        img_count_array[x_index][y_index]++;
        img_container[x_index][y_index].push_back(point_2d);
    }
    /* 计算梯度（TODO 并没有用这种梯度计算方式） */
    for (int x = 0; x < x_axis_len; ++x) {
        for (int y = 0; y < y_axis_len; ++y) {
            double gradient = 0;
            int cnt = 0;
            for (int x_inc = -1; x_inc <= 1; ++x_inc) {
                for (int y_inc = -1; y_inc <= 1; ++y_inc) {
                    int xx = x + x_inc;
                    int yy = y + y_inc;
                    if (xx >= 0 && xx < x_axis_len && yy >= 0 && yy < y_axis_len) {
                        if (xx != x || yy != y) {
                            if (img_count_array[xx][yy] >= 0) {
                                gradient += img_count_array[x][y] - img_count_array[xx][yy];
                                cnt++;
                            }
                        }
                    }
                }
            }
            if (cnt != 0) {
                gradient_array[x][y] = gradient * 1.0 / cnt;
            } else {
                gradient_array[x][y] = 0;
            }
        }
    }
    /* extract corner by gradient */
    std::vector<int> max_gradient_vec;
    std::vector<int> max_gradient_x_index_vec;
    std::vector<int> max_gradient_y_index_vec;
    // 确定最大梯度
    for (int x_segment_index = 0; x_segment_index < x_segment_num; ++x_segment_index) {
        for (int y_segment_index = 0; y_segment_index < y_segment_num; ++y_segment_index) {
            double max_gradient = 0;
            int max_gradient_x_index = -10;
            int max_gradient_y_index = -10;
            for (int x_index = x_segment_index * segment_base_num;
                 x_index < (x_segment_index + 1) * segment_base_num; x_index++) {
                for (int y_index = y_segment_index * segment_base_num;
                     y_index < (y_segment_index + 1) * segment_base_num; y_index++) {
                    if (img_count_array[x_index][y_index] > max_gradient) {
                        max_gradient = img_count_array[x_index][y_index];
                        max_gradient_x_index = x_index;
                        max_gradient_y_index = y_index;
                    }
                }
            }
            if (max_gradient >= config_setting_.corner_thre_) {
                max_gradient_vec.push_back(static_cast<int>(max_gradient));
                max_gradient_x_index_vec.push_back(max_gradient_x_index);
                max_gradient_y_index_vec.push_back(max_gradient_y_index);
            }
        }
    }
    // filter out line
    // calc line or not
    std::vector<Eigen::Vector2i> direction_list;
    Eigen::Vector2i d(0, 1);
    direction_list.push_back(d);
    d << 1, 0;
    direction_list.push_back(d);
    d << 1, 1;
    direction_list.push_back(d);
    d << 1, -1;
    direction_list.push_back(d);
    for (size_t i = 0; i < max_gradient_vec.size(); i++) {
        bool is_add = true;
        // 检查5 * 5邻域
        // 从体素中过滤图像中的线条，如果存在线条，相邻像素的值应该是相似的。
        for (int j = 0; j < 4; j++) {
            Eigen::Vector2i p(max_gradient_x_index_vec[i], max_gradient_y_index_vec[i]);
            Eigen::Vector2i p1 = p + direction_list[j];
            Eigen::Vector2i p2 = p - direction_list[j];
            int threshold = static_cast<int>(img_count_array[p[0]][p[1]] / 2);
            if (img_count_array[p1[0]][p1[1]] >= threshold && img_count_array[p2[0]][p2[1]] >= threshold) {
                // When uncommenting this line, the code will filter out corner points on the lines.
                // But this step is still not stable enough, so it is not enabled.
                // is_add = false;
            } else {
                continue;
            }
        }
        if (is_add) {
            double px = mean_x_array[max_gradient_x_index_vec[i]][max_gradient_y_index_vec[i]] /
                        img_count_array[max_gradient_x_index_vec[i]][max_gradient_y_index_vec[i]];
            double py = mean_y_array[max_gradient_x_index_vec[i]][max_gradient_y_index_vec[i]] /
                        img_count_array[max_gradient_x_index_vec[i]][max_gradient_y_index_vec[i]];
            // reproject on 3D space
            Eigen::Vector3d coord = py * x_axis + px * y_axis + proj_center;
            pcl::PointXYZINormal pi;
            pi.x = coord[0];
            pi.y = coord[1];
            pi.z = coord[2];
            pi.intensity = max_gradient_vec[i];
            pi.normal_x = static_cast<float>(proj_normal[0]);
            pi.normal_y = static_cast<float>(proj_normal[1]);
            pi.normal_z = static_cast<float>(proj_normal[2]);
            corner_points->points.push_back(pi);
        }
    }
}

/* 筛掉梯度小于近邻点的梯度的点 */
void STDescManager::non_maxi_suppression(pcl::PointCloud<pcl::PointXYZINormal>::Ptr &corner_points) const {
    std::vector<bool> is_add_vec;
    pcl::PointCloud<pcl::PointXYZINormal>::Ptr prepare_key_cloud(new pcl::PointCloud<pcl::PointXYZINormal>);
    pcl::KdTreeFLANN<pcl::PointXYZINormal> kd_tree;
    for (auto pi: corner_points->points) {
        prepare_key_cloud->push_back(pi);
        is_add_vec.push_back(true);
    }
    kd_tree.setInputCloud(prepare_key_cloud);
    std::vector<int> pointIdxRadiusSearch;
    std::vector<float> pointRadiusSquaredDistance;
    double radius = config_setting_.non_max_suppression_radius_;
    for (size_t i = 0; i < prepare_key_cloud->size(); ++i) {
        pcl::PointXYZINormal searchPoint = prepare_key_cloud->points[i];
        // KD树实现半径R内近邻搜索
        // 1. 当原点云与目标点云重叠区域较大时，优先选择nearestKSearch接口来寻找最近邻，如果点云较稀疏，并且搜索半径较小，
        //      也可以使用radiusSearch接口来寻找最近邻
        // 2. 当原点云与目标点云重叠区域较小时，优先选择radiusSearch接口来寻找最近邻，避免退化问题
        if (kd_tree.radiusSearch(searchPoint, radius, pointIdxRadiusSearch, pointRadiusSquaredDistance) > 0) {
            Eigen::Vector3d pi(searchPoint.x, searchPoint.y, searchPoint.z);
            for (auto idx: pointIdxRadiusSearch) {
                Eigen::Vector3d pj(prepare_key_cloud->points[idx].x, prepare_key_cloud->points[idx].y,
                                   prepare_key_cloud->points[idx].z);
                if (idx == i) continue;
                if (prepare_key_cloud->points[i].intensity <= prepare_key_cloud->points[idx].intensity) {
                    is_add_vec[i] = false;
                }
            }
        }
    }
    corner_points->clear();
    for (size_t i = 0; i < is_add_vec.size(); ++i) {
        if (is_add_vec[i]) {
            corner_points->points.push_back(prepare_key_cloud->points[i]);
        }
    }
}

void STDescManager::build_stdesc(const pcl::PointCloud<pcl::PointXYZINormal>::Ptr &corner_points,
                                 std::vector<STDesc> &stds_vec) const {
    stds_vec.clear();
    double scale = 1.0 / config_setting_.std_side_resolution_;
    int near_num = config_setting_.descriptor_near_num_;
    double max_dis_threshold = config_setting_.descriptor_max_len_;
    double min_dis_threshold = config_setting_.descriptor_min_len_;
    std::unordered_map<VOXEL_LOC, bool> feat_map;
    pcl::KdTreeFLANN<pcl::PointXYZINormal>::Ptr kd_tree(new pcl::KdTreeFLANN<pcl::PointXYZINormal>);
    kd_tree->setInputCloud(corner_points);
    std::vector<int> pointIdxNKNSearch(near_num);
    std::vector<float> pointNKNSquaredDistance(near_num);
    /* Search nearest N corner points to form stds. */
    /* 三个点构成STD： i, m, n(p1, p2, p3) */
    for (size_t i = 0; i < corner_points->size(); ++i) {
        pcl::PointXYZINormal searchPoint = corner_points->points[i];
        if (kd_tree->nearestKSearch(searchPoint, near_num, pointIdxNKNSearch, pointNKNSquaredDistance) > 0) {
            for (int m = 1; m < near_num; ++m) {
                for (int n = m + 1; n < near_num; ++n) {
                    pcl::PointXYZINormal p1 = searchPoint;
                    pcl::PointXYZINormal p2 = corner_points->points[pointIdxNKNSearch[m]];
                    pcl::PointXYZINormal p3 = corner_points->points[pointIdxNKNSearch[n]];
                    // 边长
                    double a = sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2) + pow(p1.z - p2.z, 2));
                    double b = sqrt(pow(p1.x - p3.x, 2) + pow(p1.y - p3.y, 2) + pow(p1.z - p3.z, 2));
                    double c = sqrt(pow(p3.x - p2.x, 2) + pow(p3.y - p2.y, 2) + pow(p3.z - p2.z, 2));
                    // 边长限制
                    if (a > max_dis_threshold || b > max_dis_threshold || c > max_dis_threshold ||
                        a < min_dis_threshold || b < min_dis_threshold || c < min_dis_threshold)
                        continue;
                    /* 根据边长关系重新对顶点进行编号 */
                    double temp;
                    Eigen::Vector3d A, B, C;
                    Eigen::Vector3i l1, l2, l3; // 顶点标签
                    Eigen::Vector3i l_temp;
                    /* 表示边上两个顶点的编号 */
                    l1 << 1, 2, 0;
                    l2 << 1, 0, 3;
                    l3 << 0, 2, 3;
                    if (a > b) {
                        temp = a;
                        a = b;
                        b = temp;
                        l_temp = l1;
                        l1 = l2;
                        l2 = l_temp;
                    }
                    if (b > c) {
                        temp = b;
                        b = c;
                        c = temp;
                        l_temp = l2;
                        l2 = l3;
                        l3 = l_temp;
                    }
                    if (a > b) {
                        temp = a;
                        a = b;
                        b = temp;
                        l_temp = l1;
                        l1 = l2;
                        l2 = l_temp;
                    }
                    // check augnmentation
                    pcl::PointXYZ d_p;
                    d_p.x = static_cast<float>(a * 1000);
                    d_p.y = static_cast<float>(b * 1000);
                    d_p.z = static_cast<float>(c * 1000);
                    VOXEL_LOC position((int64_t) d_p.x, (int64_t) d_p.y, (int64_t) d_p.z);
                    auto iter = feat_map.find(position);
                    Eigen::Vector3d normal_1, normal_2, normal_3;
                    /* 避免加入边长相同的冗余描述子 */
                    if (iter == feat_map.end()) {
                        Eigen::Vector3d vertex_attached;
                        /* A */
                        if (l1[0] == l2[0]) {
                            A << p1.x, p1.y, p1.z;
                            normal_1 << p1.normal_x, p1.normal_y, p1.normal_z;
                            vertex_attached[0] = p1.intensity;
                        } else if (l1[1] == l2[1]) {
                            A << p2.x, p2.y, p2.z;
                            normal_1 << p2.normal_x, p2.normal_y, p2.normal_z;
                            vertex_attached[0] = p2.intensity;
                        } else {
                            A << p3.x, p3.y, p3.z;
                            normal_1 << p3.normal_x, p3.normal_y, p3.normal_z;
                            vertex_attached[0] = p3.intensity;
                        }
                        /* B */
                        if (l1[0] == l3[0]) {
                            B << p1.x, p1.y, p1.z;
                            normal_2 << p1.normal_x, p1.normal_y, p1.normal_z;
                            vertex_attached[1] = p1.intensity;
                        } else if (l1[1] == l3[1]) {
                            B << p2.x, p2.y, p2.z;
                            normal_2 << p2.normal_x, p2.normal_y, p2.normal_z;
                            vertex_attached[1] = p2.intensity;
                        } else {
                            B << p3.x, p3.y, p3.z;
                            normal_2 << p3.normal_x, p3.normal_y, p3.normal_z;
                            vertex_attached[1] = p3.intensity;
                        }
                        /* C */
                        if (l2[0] == l3[0]) {
                            C << p1.x, p1.y, p1.z;
                            normal_3 << p1.normal_x, p1.normal_y, p1.normal_z;
                            vertex_attached[2] = p1.intensity;
                        } else if (l2[1] == l3[1]) {
                            C << p2.x, p2.y, p2.z;
                            normal_3 << p2.normal_x, p2.normal_y, p2.normal_z;
                            vertex_attached[2] = p2.intensity;
                        } else {
                            C << p3.x, p3.y, p3.z;
                            normal_3 << p3.normal_x, p3.normal_y, p3.normal_z;
                            vertex_attached[2] = p3.intensity;
                        }
                        STDesc single_descriptor;
                        single_descriptor.vertex_A_ = A;
                        single_descriptor.vertex_B_ = B;
                        single_descriptor.vertex_C_ = C;
                        single_descriptor.center_ = (A + B + C) / 3;
                        single_descriptor.vertex_attached_ = vertex_attached;
                        single_descriptor.side_length_ << scale * a, scale * b, scale * c;
                        // TODO:源代码乘5,why：
                        /* 可能是为了增加角度信息的变化范围， 如果不乘以这个标量，角度信息可能会比较小，不足以描述三个面之间的差异。
                         * 通过乘以标量5，可以将角度信息的范围从0-1之间扩大到0-5之间，从而更好地描述三个面之间的差异。
                         * 这里的5应该可以根据应用场景调整，主要取决于场景需要更大还是更小的角度范围 */
                        single_descriptor.angle_[0] = fabs(5 * normal_1.dot(normal_2));
                        single_descriptor.angle_[1] = fabs(5 * normal_1.dot(normal_3));
                        single_descriptor.angle_[2] = fabs(5 * normal_3.dot(normal_2));
                        single_descriptor.frame_id_ = current_frame_id_;
//                        single_descriptor.std_time = key_frame_times[current_frame_id_];
                        feat_map[position] = true;
                        stds_vec.push_back(single_descriptor);
                    }
                }
            }
        }
    }
}

/* 搜索结果 <candidate_id, plane icp score>. <-1, 0> for no loop */
void STDescManager::SearchLoop(const std::vector<STDesc> &stds_vec, std::pair<int, double> &loop_result,
                               std::pair<Eigen::Vector3d, Eigen::Matrix3d> &loop_transform,
                               std::vector<std::pair<STDesc, STDesc>> &loop_std_pair) {
    if (stds_vec.empty()) {
        ROS_ERROR_STREAM("No STDescs!");
        loop_result = std::pair<int, double>(-1, 0);
        return;
    }
    /* step1, select candidates, default number 50 */
    auto t1 = std::chrono::high_resolution_clock::now();
    std::vector<STDMatchList> candidate_matcher_vec;
    candidate_selector(stds_vec, candidate_matcher_vec);
    auto t2 = std::chrono::high_resolution_clock::now();
    /* step2, select best candidates from rough candidates */
    double best_score = 0;
    unsigned int best_candidate_id = -1;
    unsigned int triangle_candidate = -1;
    std::pair<Eigen::Vector3d, Eigen::Matrix3d> best_transform;
    std::vector<std::pair<STDesc, STDesc>> best_success_match_vec;
    for (size_t i = 0; i < candidate_matcher_vec.size(); ++i) {
        double verify_score = -1;
        std::pair<Eigen::Vector3d, Eigen::Matrix3d> relative_pose;
        std::vector<std::pair<STDesc, STDesc>> success_match_vec;
        candidate_verify(candidate_matcher_vec[i], verify_score, relative_pose, success_match_vec);
        if (verify_score > best_score) {
            best_score = verify_score;
            best_candidate_id = candidate_matcher_vec[i].match_id_.second;
            best_transform = relative_pose;
            best_success_match_vec = success_match_vec;
            triangle_candidate = i;
        }
    }
    auto t3 = std::chrono::high_resolution_clock::now();
//    std::cout << "[Time] candidate selector: " << time_inc(t2, t1) << " ms, candidate verify: " << time_inc(t3, t2)
//              << "ms" << std::endl;
    if (best_score > config_setting_.icp_threshold_) {
        loop_result = std::pair<int, double>(best_candidate_id, best_score);
        loop_transform = best_transform;
        loop_std_pair = best_success_match_vec;
        return;
    } else {
        loop_result = std::pair<int, double>(-1, 0);
        return;
    }
}

/* Select a specified number of candidate frames according to the number of STDesc rough matches(粗略匹配选择给定数量的候选匹配帧) */
void STDescManager::candidate_selector(const std::vector<STDesc> &stds_vec,
                                       std::vector<STDMatchList> &candidate_matcher_vec) {
    double match_array[MAX_FRAME_N] = {0};
    std::vector<std::pair<STDesc, STDesc>> match_vec;
    std::vector<int> match_index_vec;
    std::vector<Eigen::Vector3i> voxel_round;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            for (int z = -1; z <= 1; z++) {
                Eigen::Vector3i voxel_inc(x, y, z);
                voxel_round.push_back(voxel_inc);
            }
        }
    }
    std::vector<bool> useful_match(stds_vec.size());
    std::vector<std::vector<size_t>> useful_match_index(stds_vec.size());
    std::vector<std::vector<STDesc_LOC>> useful_match_position(stds_vec.size());
    std::vector<size_t> index(stds_vec.size());
    for (size_t i = 0; i < index.size(); ++i) {
        index[i] = i;
        useful_match[i] = false;
    }
    /* speed up matching */
    int dis_match_cnt = 0;
    int final_match_cnt = 0;
#ifdef MP_EN
    omp_set_num_threads(MP_PROC_NUM);
#pragma omp parallel for
#endif
    for (size_t i = 0; i < stds_vec.size(); ++i) {
        STDesc src_std = stds_vec[i];
        STDesc_LOC position;
        int best_index = 0;
        STDesc_LOC best_position;
        double dis_threshold = src_std.side_length_.norm() * config_setting_.rough_dis_threshold_;
        for (auto voxel_inc: voxel_round) {
            position.x = static_cast<int>(src_std.side_length_[0] + voxel_inc[0]);
            position.y = static_cast<int>(src_std.side_length_[1] + voxel_inc[1]);
            position.z = static_cast<int>(src_std.side_length_[2] + voxel_inc[2]);
            Eigen::Vector3d voxel_center(static_cast<double>(position.x) + 0.5,
                                         static_cast<double>(position.y) + 0.5, static_cast<double>(position.z) + 0.5);
            // TODO:此处这里的判断含义？？？1.5是如何确定的
            if ((src_std.side_length_ - voxel_center).norm() < 1.5) {
                auto iter = data_base_.find(position);
                if (iter != data_base_.end()) {
                    for (size_t j = 0; j < data_base_[position].size(); ++j) {
                        if (src_std.frame_id_ - data_base_[position][j].frame_id_ > config_setting_.skip_near_num_) {
                            double dis = (src_std.side_length_ - data_base_[position][j].side_length_).norm();
                            // rough filter with side lengths
                            if (dis < dis_threshold) {
                                dis_match_cnt++;
                                // rough filter with vertex attached info
                                double vertex_attach_diff = 2 * (src_std.vertex_attached_ -
                                                                 data_base_[position][j].vertex_attached_).norm() /
                                                            (src_std.vertex_attached_ +
                                                             data_base_[position][j].vertex_attached_).norm();
//                                std::cout << "vertex diff:" << vertex_attach_diff << std::endl;
                                if (vertex_attach_diff < config_setting_.vertex_diff_threshold_) {
                                    // TODO 增加时间限制，当两帧时间差大于一个阈值，才去作为候选帧
//                                    if (abs(src_std.std_time - data_base_[position][j].std_time) >
//                                        config_setting_.historyKeyframeSearchTimeDiff) {
//                                        std::cout << "target: " << src_std.std_time << "source: "
//                                                  << data_base_[position][j].std_time << std::endl;
                                        final_match_cnt++;
                                        useful_match[i] = true;
                                        useful_match_position[i].push_back(position);
                                        useful_match_index[i].push_back(j);
//                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    std::cout << "dis match num:" << dis_match_cnt << ", final match num:" << final_match_cnt << std::endl;
    /* record match index */
    std::vector<Eigen::Vector2i, Eigen::aligned_allocator<Eigen::Vector2i>> index_recorder;
    for (size_t i = 0; i < useful_match.size(); ++i) {
        if (useful_match[i]) {
            for (size_t j = 0; j < useful_match_index[i].size(); ++j) {
                // vote +1
                match_array[data_base_[useful_match_position[i][j]][useful_match_index[i][j]].frame_id_] += 1;
                Eigen::Vector2i match_index(i, j);
                index_recorder.push_back(match_index);
                match_index_vec.push_back(data_base_[useful_match_position[i][j]][useful_match_index[i][j]].frame_id_);
            }
        }
    }

    /* select candidate according to the matching score */
    for (int cnt = 0; cnt < config_setting_.candidate_num_; ++cnt) {
        double max_vote = 1;
        int max_vote_index = -1; // 帧数索引
        for (int i = 0; i < MAX_FRAME_N; ++i) {
            if (match_array[i] > max_vote) {
                max_vote = match_array[i];
                max_vote_index = i;
            }
        }
        STDMatchList match_triangle_list;
        if (max_vote_index >= 0 && max_vote >= 5) {
            // 如果匹配对绑定，则投票数清零，以便下一次选取候选帧
            match_array[max_vote_index] = 0;
            // 匹配对索引绑定
            match_triangle_list.match_id_.first = current_frame_id_;
            match_triangle_list.match_id_.second = max_vote_index;
            // 遍历每一组匹配成功的描述子对
            for (size_t i = 0; i < index_recorder.size(); ++i) {
                // 如果是投票数最大的帧
                if (match_index_vec[i] == max_vote_index) {
                    std::pair<STDesc, STDesc> single_match_pair;
                    // 0,1对应match_index(i, j)里的i, j
                    single_match_pair.first = stds_vec[index_recorder[i][0]];
                    single_match_pair.second =
                            data_base_[useful_match_position[index_recorder[i][0]][index_recorder[i][1]]]
                            [useful_match_index[index_recorder[i][0]][index_recorder[i][1]]];
                    match_triangle_list.match_list_.push_back(single_match_pair);
                }
            }
            candidate_matcher_vec.push_back(match_triangle_list);
        } else {
            break;
        }
    }
}

/* 通过几何验证找到最优候选帧 */
void STDescManager::candidate_verify(const STDMatchList &candidate_matcher, double &verify_score,
                                     std::pair<Eigen::Vector3d, Eigen::Matrix3d> &relative_pose,
                                     std::vector<std::pair<STDesc, STDesc>> &success_match_vec) {
    success_match_vec.clear();
    int skip_len = static_cast<int>(candidate_matcher.match_list_.size() / 50) + 1;
    int use_size = candidate_matcher.match_list_.size() / skip_len;
    double dis_threshold = 3.0;
    std::vector<size_t> index(use_size);
    std::vector<int> vote_list(use_size);
    for (size_t i = 0; i < index.size(); i++) {
        index[i] = i;
    }
    std::mutex mylock;
#ifdef MP_EN
    omp_set_num_threads(MP_PROC_NUM);
#pragma omp parallel for
#endif
    for (size_t i = 0; i < use_size; ++i) {
        auto single_pair = candidate_matcher.match_list_[i * skip_len];
        int vote = 0;
        Eigen::Matrix3d test_rot;
        Eigen::Vector3d test_t;
        triangle_solver(single_pair, test_t, test_rot);
        for (const auto &verify_pair: candidate_matcher.match_list_) {
            Eigen::Vector3d A = verify_pair.first.vertex_A_;
            Eigen::Vector3d A_transform = test_rot * A + test_t;
            Eigen::Vector3d B = verify_pair.first.vertex_B_;
            Eigen::Vector3d B_transform = test_rot * B + test_t;
            Eigen::Vector3d C = verify_pair.first.vertex_C_;
            Eigen::Vector3d C_transform = test_rot * C + test_t;
            // 公式(4)
            double dis_A = (A_transform - verify_pair.second.vertex_A_).norm();
            double dis_B = (B_transform - verify_pair.second.vertex_B_).norm();
            double dis_C = (C_transform - verify_pair.second.vertex_C_).norm();
            if (dis_A < dis_threshold && dis_B < dis_threshold && dis_C < dis_threshold) {
                vote++;
            }
        }
        mylock.lock();
        vote_list[i] = vote;
        mylock.unlock();
    }
    size_t max_vote_index = 0;
    int max_vote = 0;
    for (size_t i = 0; i < vote_list.size(); ++i) {
        if (max_vote < vote_list[i]) {
            max_vote = vote_list[i];
            max_vote_index = i;
        }
    }
    /* 上面部分相当于找到最大化正确匹配描述子数量的转换 */
    if (max_vote >= 4) {
        auto best_pair = candidate_matcher.match_list_[max_vote_index * skip_len];
        int vote = 0;
        Eigen::Matrix3d best_rot;
        Eigen::Vector3d best_t;
        triangle_solver(best_pair, best_t, best_rot);
        relative_pose.first = best_t;
        relative_pose.second = best_rot;
        for (const auto &verify_pair: candidate_matcher.match_list_) {
            Eigen::Vector3d A = verify_pair.first.vertex_A_;
            Eigen::Vector3d A_transform = best_rot * A + best_t;
            Eigen::Vector3d B = verify_pair.first.vertex_B_;
            Eigen::Vector3d B_transform = best_rot * B + best_t;
            Eigen::Vector3d C = verify_pair.first.vertex_C_;
            Eigen::Vector3d C_transform = best_rot * C + best_t;
            double dis_A = (A_transform - verify_pair.second.vertex_A_).norm();
            double dis_B = (B_transform - verify_pair.second.vertex_B_).norm();
            double dis_C = (C_transform - verify_pair.second.vertex_C_).norm();
            if (dis_A < dis_threshold && dis_B < dis_threshold && dis_C < dis_threshold) {
                success_match_vec.push_back(verify_pair);
            }
        }
        /* 计算平面重合分数 */
        verify_score = plane_geometric_verify(plane_cloud_vec_.back(),
                                              plane_cloud_vec_[candidate_matcher.match_id_.second], relative_pose);
    } else {
        verify_score = -1;
    }
}

/* SVD分解得到一个匹配对之间的变换关系 */
void STDescManager::triangle_solver(std::pair<STDesc, STDesc> &std_pair, Eigen::Vector3d &t, Eigen::Matrix3d &rot) {
    Eigen::Matrix3d src = Eigen::Matrix3d::Zero();
    Eigen::Matrix3d ref = Eigen::Matrix3d::Zero();
    // 去质心
    src.col(0) = std_pair.first.vertex_A_ - std_pair.first.center_;
    src.col(1) = std_pair.first.vertex_B_ - std_pair.first.center_;
    src.col(2) = std_pair.first.vertex_C_ - std_pair.first.center_;
    ref.col(0) = std_pair.second.vertex_A_ - std_pair.second.center_;
    ref.col(1) = std_pair.second.vertex_B_ - std_pair.second.center_;
    ref.col(2) = std_pair.second.vertex_C_ - std_pair.second.center_;
    Eigen::Matrix3d covariance = src * ref.transpose();
    Eigen::JacobiSVD<Eigen::MatrixXd> svd(covariance, Eigen::ComputeThinU | Eigen::ComputeThinV);
    Eigen::Matrix3d V = svd.matrixV();
    Eigen::Matrix3d U = svd.matrixU();
    rot = V * U.transpose();
    if (rot.determinant() < 0) {
        Eigen::Matrix3d K;
        K << 1, 0, 0, 0, 1, 0, 0, 0, -1;
        rot = V * K * U.transpose();
    }
    t = -rot * std_pair.first.center_ + std_pair.second.center_;
}

double STDescManager::plane_geometric_verify(const pcl::PointCloud<pcl::PointXYZINormal>::Ptr &source_cloud,
                                             const pcl::PointCloud<pcl::PointXYZINormal>::Ptr &target_cloud,
                                             const std::pair<Eigen::Vector3d, Eigen::Matrix3d> &transform) const {
    Eigen::Vector3d t = transform.first;
    Eigen::Matrix3d rot = transform.second;
    pcl::KdTreeFLANN<pcl::PointXYZ>::Ptr kd_tree(new pcl::KdTreeFLANN<pcl::PointXYZ>);
    pcl::PointCloud<pcl::PointXYZ>::Ptr input_cloud(new pcl::PointCloud<pcl::PointXYZ>);
    for (size_t i = 0; i < target_cloud->size(); i++) {
        pcl::PointXYZ pi;
        pi.x = target_cloud->points[i].x;
        pi.y = target_cloud->points[i].y;
        pi.z = target_cloud->points[i].z;
        input_cloud->push_back(pi);
    }
    kd_tree->setInputCloud(input_cloud);
    std::vector<int> pointIdxNKNSearch(1);
    std::vector<float> pointNKNSquaredDistance(1);
    double useful_match = 0;
    double normal_threshold = config_setting_.normal_threshold_;
    double dis_threshold = config_setting_.dis_threshold_;
    for (size_t i = 0; i < source_cloud->size(); i++) {
        pcl::PointXYZINormal searchPoint = source_cloud->points[i];
        pcl::PointXYZ use_search_point;
        use_search_point.x = searchPoint.x;
        use_search_point.y = searchPoint.y;
        use_search_point.z = searchPoint.z;
        Eigen::Vector3d pi(searchPoint.x, searchPoint.y, searchPoint.z);
        pi = rot * pi + t;
        use_search_point.x = pi[0];
        use_search_point.y = pi[1];
        use_search_point.z = pi[2];
        Eigen::Vector3d ni(searchPoint.normal_x, searchPoint.normal_y, searchPoint.normal_z);
        ni = rot * ni;
        int K = 3;
        if (kd_tree->nearestKSearch(use_search_point, K, pointIdxNKNSearch, pointNKNSquaredDistance) > 0) {
            for (size_t j = 0; j < K; ++j) {
                pcl::PointXYZINormal nearstPoint = target_cloud->points[pointIdxNKNSearch[j]];
                Eigen::Vector3d tpi(nearstPoint.x, nearstPoint.y, nearstPoint.z);
                Eigen::Vector3d tni(nearstPoint.normal_x, nearstPoint.normal_y, nearstPoint.normal_z);
                Eigen::Vector3d normal_inc = ni - tni;
                Eigen::Vector3d normal_add = ni + tni;
                double point_to_plane = fabs(tni.transpose() * (pi - tpi));
                if ((normal_inc.norm() < normal_threshold || normal_add.norm() < normal_threshold) &&
                    point_to_plane < dis_threshold) {
                    useful_match++;
                    break;
                }
            }
        }
    }
    return useful_match / source_cloud->size();
}

void STDescManager::AddSTDescs(const std::vector<STDesc> &stds_vec) {
    // update frame id
    current_frame_id_++;
    for (auto single_std: stds_vec) {
        STDesc_LOC position;
        /* TODO WARNING:直接(int)(double + 0.5)是有问题的，数字0.499999975（低于0.5的最小可表示的浮点数）舍入到1.0。
         * 负数的行为更糟糕，-0.5f和-1.4f都四舍五入为0.0 */
//        position.x = (int) (single_std.side_length_[0] + 0.5);
        position.x = lround(single_std.side_length_[0] + 0.5);
        position.y = lround(single_std.side_length_[1] + 0.5);
        position.z = lround(single_std.side_length_[2] + 0.5);
        position.a = static_cast<int>(single_std.angle_[0]);
        position.b = static_cast<int>(single_std.angle_[1]);
        position.c = static_cast<int>(single_std.angle_[2]);
        auto iter = data_base_.find(position);
        if (iter != data_base_.end()) {
            data_base_[position].push_back(single_std);
        } else {
            std::vector<STDesc> descriptor_vec;
            descriptor_vec.push_back(single_std);
            data_base_[position] = descriptor_vec;
        }
    }
}

/* Geometrical optimization by plane-to-plane icp
 * 论文中所说的进一步优化(4)中的法向量差和点面距离，以获得更精确的回环校正变换关系（STD-ICP） */
void STDescManager::PlaneGeomrtricIcp(const pcl::PointCloud<pcl::PointXYZINormal>::Ptr &source_cloud,
                                      const pcl::PointCloud<pcl::PointXYZINormal>::Ptr &target_cloud,
                                      std::pair<Eigen::Vector3d, Eigen::Matrix3d> &transform) const {
    pcl::KdTreeFLANN<pcl::PointXYZ>::Ptr kd_tree(new pcl::KdTreeFLANN<pcl::PointXYZ>);
    pcl::PointCloud<pcl::PointXYZ>::Ptr input_cloud(new pcl::PointCloud<pcl::PointXYZ>);
    for (size_t i = 0; i < target_cloud->size(); i++) {
        pcl::PointXYZ pi;
        pi.x = target_cloud->points[i].x;
        pi.y = target_cloud->points[i].y;
        pi.z = target_cloud->points[i].z;
        input_cloud->push_back(pi);
    }
    kd_tree->setInputCloud(input_cloud); // target_cloud
    ceres::Manifold *quaternion_manifold = new ceres::EigenQuaternionManifold;
    ceres::Problem problem;
    ceres::LossFunction *loss_function = nullptr;
    Eigen::Matrix3d rot = transform.second;
    Eigen::Quaterniond q(rot); // w,x,y,z
    Eigen::Vector3d t = transform.first;
    double para_q[4] = {q.x(), q.y(), q.z(), q.w()};
    double para_t[3] = {t(0), t(1), t(2)};
    problem.AddParameterBlock(para_q, 4, quaternion_manifold);
    problem.AddParameterBlock(para_t, 3);
    Eigen::Map<Eigen::Quaterniond> q_last_curr(para_q);
    Eigen::Map<Eigen::Vector3d> t_last_curr(para_t);
    std::vector<int> pointIdxNKNSearch(1);
    std::vector<float> pointNKNSquaredDistance(1);
    int useful_match = 0;
    for (size_t i = 0; i < source_cloud->size(); ++i) {
        pcl::PointXYZINormal searchPoint = source_cloud->points[i];
        Eigen::Vector3d pi(searchPoint.x, searchPoint.y, searchPoint.z);
        pi = rot * pi + t;
        pcl::PointXYZ use_search_point;
        use_search_point.x = pi[0];
        use_search_point.y = pi[1];
        use_search_point.z = pi[2];
        Eigen::Vector3d ni(searchPoint.normal_x, searchPoint.normal_y,
                           searchPoint.normal_z);
        ni = rot * ni;
        if (kd_tree->nearestKSearch(use_search_point, 1, pointIdxNKNSearch, pointNKNSquaredDistance) > 0) {
            pcl::PointXYZINormal nearstPoint = target_cloud->points[pointIdxNKNSearch[0]];
            Eigen::Vector3d tpi(nearstPoint.x, nearstPoint.y, nearstPoint.z);
            Eigen::Vector3d tni(nearstPoint.normal_x, nearstPoint.normal_y, nearstPoint.normal_z);
            Eigen::Vector3d normal_inc = ni - tni;
            Eigen::Vector3d normal_add = ni + tni;
            double point_to_point_dis = (pi - tpi).norm();
            double point_to_plane = fabs(tni.transpose() * (pi - tpi));
            if (normal_inc.norm() < config_setting_.normal_threshold_ ||
                normal_add.norm() < config_setting_.normal_threshold_ &&
                point_to_plane < config_setting_.dis_threshold_ && point_to_point_dis < 3) {
                useful_match++;
                ceres::CostFunction *cost_function;
                Eigen::Vector3d curr_point(source_cloud->points[i].x,
                                           source_cloud->points[i].y,
                                           source_cloud->points[i].z);
                Eigen::Vector3d curr_normal(source_cloud->points[i].normal_x,
                                            source_cloud->points[i].normal_y,
                                            source_cloud->points[i].normal_z);
                cost_function = PlaneSolver::Create(curr_point, curr_normal, tpi, tni);
                // 创建了一个残差块，表示当前平面和目标平面在经过旋转和平移参数转换后的误差。
                // ceres::LossFunction对象被设置为nullptr，所以没有对残差进行加权处理。
                problem.AddResidualBlock(cost_function, loss_function, para_q, para_t);
            }
        }
    }
    /* ceres优化求解 cholesky分解；最大迭代次数：100；进度不会打印到控制台输出； */
    ceres::Solver::Options options;
    options.linear_solver_type = ceres::SPARSE_NORMAL_CHOLESKY; // cholesky分解，用于具有稀疏性的大规模非线性最小二乘问题求解
    options.max_num_iterations = 100;
    options.minimizer_progress_to_stdout = false;
    ceres::Solver::Summary summary;
    ceres::Solve(options, &problem, &summary);
    // 取出求解后最优的变换关系
    Eigen::Quaterniond q_opt(para_q[3], para_q[0], para_q[1], para_q[2]);
    rot = q_opt.toRotationMatrix();
    t << t_last_curr(0), t_last_curr(1), t_last_curr(2);
    transform.first = t;
    transform.second = rot;
    // std::cout << "useful match for icp:" << useful_match << std::endl;
}

/* 初始化平面，平面判断 */
void OctoTree::init_plane() {
    plane_ptr_->covariance_ = Eigen::Matrix3d::Zero();
    plane_ptr_->center_ = Eigen::Vector3d::Zero();
    plane_ptr_->normal_ = Eigen::Vector3d::Zero();
    plane_ptr_->points_size_ = voxel_points_.size();
    plane_ptr_->radius_ = 0;
    for (auto pi: voxel_points_) {
        plane_ptr_->covariance_ += pi * pi.transpose();
        plane_ptr_->center_ += pi;
    }
    plane_ptr_->center_ = plane_ptr_->center_ / plane_ptr_->points_size_; // 中点
    // 协方差
    plane_ptr_->covariance_ =
            plane_ptr_->covariance_ / plane_ptr_->points_size_ - plane_ptr_->center_ * plane_ptr_->center_.transpose();
    // 得到协方差矩阵的特征值和特征向量
    Eigen::EigenSolver<Eigen::Matrix3d> es(plane_ptr_->covariance_);
    Eigen::Matrix3cd evecs = es.eigenvectors();
    Eigen::Vector3cd evals = es.eigenvalues();
    Eigen::Vector3d evalsReal;
    evalsReal = evals.real(); // 实数部分
    Eigen::Matrix3d::Index evalsMin, evalsMax;
    evalsReal.rowwise().sum().minCoeff(&evalsMin);
    evalsReal.rowwise().sum().maxCoeff(&evalsMax);
    int evalsMid = 3 - evalsMin - evalsMax;
    // 平面判定
    if (evalsReal(evalsMin) < config_setting_.plane_detection_thre_) {
        plane_ptr_->normal_ << evecs.real()(0, evalsMin), evecs.real()(1, evalsMin), evecs.real()(2, evalsMin);
        plane_ptr_->min_eigen_value_ = evalsReal(evalsMin);
        plane_ptr_->radius_ = static_cast<float>(sqrt(evalsReal(evalsMax)));
        plane_ptr_->is_plane_ = true;
        // 计算点面匹配残差时的第二部分
        plane_ptr_->intercept_ = static_cast<float>(-(plane_ptr_->normal_(0) * plane_ptr_->center_(0) +
                                                      plane_ptr_->normal_(1) * plane_ptr_->center_(1) +
                                                      plane_ptr_->normal_(2) * plane_ptr_->center_(2)));
        plane_ptr_->p_center_.x = plane_ptr_->center_(0);
        plane_ptr_->p_center_.y = plane_ptr_->center_(1);
        plane_ptr_->p_center_.z = plane_ptr_->center_(2);
        plane_ptr_->p_center_.normal_x = plane_ptr_->normal_(0);
        plane_ptr_->p_center_.normal_y = plane_ptr_->normal_(1);
        plane_ptr_->p_center_.normal_z = plane_ptr_->normal_(2);
    } else {
        plane_ptr_->is_plane_ = false;
    }
}

/* 初始化体素 */
void OctoTree::init_octo_tree() {
    if (voxel_points_.size() > config_setting_.voxel_init_num_) {
        init_plane();
    }
}

