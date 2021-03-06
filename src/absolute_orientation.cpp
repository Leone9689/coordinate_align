#include "absolute_orientation.h"
#include<iostream>
#include<algorithm>
#include<fstream>
#include <iomanip>
#include <Eigen/Eigenvalues>
using namespace std;
 void Coordinate_Align::input_datas(const string &keyframe_pose,const string &groundtruth_pose)
 {
     ifstream key_stream,ground_stream;
     string tempStr;
     PoseDatas key_pose,ground_pose;

     key_stream.open(keyframe_pose);
     while(!key_stream.eof())     //acquare keyframe dataset
   {
     getline(key_stream, tempStr);
     if(key_stream.fail())
         break;
     key_stream>>key_pose.time>>key_pose.position(0)>>key_pose.position(1)>>key_pose.position(2);
     key_dataset.push_back(key_pose);
   }
    ground_stream.open(groundtruth_pose);
    while(!ground_stream.eof())   //acquare groundtruth dataset
   {
       getline(ground_stream, tempStr);
        if(ground_stream.fail())
            break;
        ground_stream>>ground_pose.time>>ground_pose.position(0)>>ground_pose.position(1)>>ground_pose.position(2);
        ground_dataset.push_back(ground_pose);
   }
  time_synchronization(key_dataset,ground_dataset);

 }
 ///
 /// \brief Coordinate_Align::time_synchronization
 /// \param key_dataset
 /// \param ground_dataset
 ///   acquare groundtruth dataset time-synchronization with keyframe dataset
void  Coordinate_Align::time_synchronization(const vector<PoseDatas> &key_dataset, const vector<PoseDatas> &ground_dataset)
{
  int j=0;
  n=key_dataset.size()-8;// duo to my the most time of keyframe dataset more than the most time of groundtruth dataset
  ofstream f;
  f.open("new_groundtruth.txt");
  f << fixed;
  for(int i=0;i<n;i++)
      {
        while(ground_dataset[j].time-key_dataset[i].time<=0)
            {
             temporary_dataset.push_back(ground_dataset[j]);
             j++;

            }
        some_ground_dataset.push_back(temporary_dataset.back());
        f << setprecision(6) <<temporary_dataset.back().time<< " " << setprecision(9) <<temporary_dataset.back().position(0)<< " "
          << temporary_dataset.back().position(1)<< " " << temporary_dataset.back().position(2)<< endl;
      }
  compute_q(key_dataset,some_ground_dataset);
}
void Coordinate_Align::compute_q(const vector<PoseDatas> &key_dataset, const vector<PoseDatas> &ground_dataset)
{
    Eigen::Matrix<double,3,1> rl_err,rr_err;
    vector<Eigen::Matrix<double,3,1>>rl,rr;
    Eigen::Matrix<double,1,3> rr_transpose;
    double Sxx,Sxy,Sxz;
    double Syx,Syy,Syz;
    double Szx,Szy,Szz;
    double max_eigenvalue=0;
    for(int i=0;i<n;i++)
       {
        rl_err=rl_err+key_dataset[i].position;
        rr_err=rr_err+ground_dataset[i].position;
       }
       rl_ave=rl_err/n;
       rr_ave=rr_err/n;
    for(int j=0;j<n;j++)
        {
         rl_err=key_dataset[j].position-rl_ave;
         rr_err=ground_dataset[j].position-rr_ave;
         rl.push_back(rl_err);
         rr.push_back(rr_err);
         }
     for(int i=0; i<n;i++)   //acquare M
        {
         rr_transpose(0)=rr[i](0);
         rr_transpose(1)=rr[i](1);
         rr_transpose(2)=rr[i](2);
        M=M+rl[i]*rr_transpose;
       }
     Sxx=M(0,0);
     Sxy=M(0,1);
     Sxz=M(0,2);
     Syx=M(1,0);
     Syy=M(1,1);
     Syz=M(1,2);
     Szx=M(2,0);
     Szy=M(2,1);
     Szz=M(2,2);

     N(0,0)=Sxx+Syy+Szz;
     N(0,1)=Syz-Szy;
     N(0,2)=Szx-Sxz;
     N(0,3)=Sxy-Syx;
     N(1,0)=Syz-Szy;
     N(1,1)=Sxx-Syy-Szz;
     N(1,2)=Sxy+Syx;
     N(1,3)=Szx+Sxz;
     N(2,0)=Szx-Sxz;
     N(2,1)=Sxy+Syx;
     N(2,2)=-Sxx+Syy-Szz;
     N(2,3)=Syz+Szy;
     N(3,0)=Sxy-Syx;
     N(3,1)=Szx+Sxz;
     N(3,2)=Syz+Szy;
     N(3,3)=-Sxx-Syy+Szz;
     Eigen::EigenSolver<Eigen::Matrix4d> es(N);
     Eigen::Matrix4d D = es.pseudoEigenvalueMatrix();
     Eigen::Matrix4d V = es.pseudoEigenvectors();
     for(int i=0;i<4;i++)
         {
         if(D(i,i)>max_eigenvalue)
         {
         max_eigenvalue= D(i,i);
         q.w()= V(0,i);
         q.vec()(0)=V(1,i);
         q.vec()(1)=V(2,i);
         q.vec()(2)=V(3,i);
         q.normalized();
         }
         }
     cout<<"q= "<<q.w()<<" "<<q.vec()(0)<<" "<<q.vec()(1)<<" "<<q.vec()(2)<<endl;
     compute_s(rl,rr);
     }
void Coordinate_Align::compute_s(const vector<Eigen::Matrix<double,3,1>> &rl,const vector<Eigen::Matrix<double,3,1>> &rr)
{
    double Sr=0,Sl=0,s;
    for(int i=0;i<n;i++)
        {
         Sr=Sr+sqrt(rr[i](0)*rr[i](0)+rr[i](1)*rr[i](1)+rr[i](2)*rr[i](2));
         Sl=Sl+sqrt(rl[i](0)*rl[i](0)+rl[i](1)*rl[i](1)+rl[i](2)*rl[i](2));
        }
    s=sqrt(Sr/Sl);
    cout<<"s= "<<s<<endl;
   compute_p(s,q,rl_ave,rr_ave);

}
void Coordinate_Align::compute_p(const double &s,const Eigen::Quaterniond &q,const Eigen::Matrix<double,3,1> &rl_ave,const Eigen::Matrix<double,3,1> &rr_ave)
{
    Eigen::Matrix<double,3,1> p;
    double error_x,error_y,error_z,error_ave; //estimate value-true value
    //Eigen::Matrix<double,3,1> new_keyframe_pose;
    ofstream f,ff;
    f.open("new_keyframe.txt");
    ff.open("error.txt");
    f << fixed;
    ff << fixed;
    p=rr_ave-s*q.toRotationMatrix()*rl_ave;
    cout<<"p= "<<p<<endl;
    for(int i=0;i<n;i++)
      {
       key_dataset[i].position=s*q.toRotationMatrix()*key_dataset[i].position+p;
       error_x=key_dataset[i].position(0)-some_ground_dataset[i].position(0);
       error_y=key_dataset[i].position(1)-some_ground_dataset[i].position(1);
       error_z=key_dataset[i].position(2)-some_ground_dataset[i].position(2);
       error_ave=sqrt(error_x*error_x+error_y*error_y+error_z*error_z);

       f << setprecision(6) <<key_dataset[i].time<< " " << setprecision(9) <<key_dataset[i].position(0)<< " "
         <<key_dataset[i].position(1)<< " " << key_dataset[i].position(2)<< endl;

       ff<< setprecision(6) <<key_dataset[i].time<< " " << setprecision(9)<<error_x<<" "
          <<error_y<<" "<<error_z<<" "<<error_ave<<endl;
      }
}
