//
// Created by TD on 25-7-21.
//

#pragma once

#include <maya_plug/maya_plug_fwd.h>

#include <maya/MPxNode.h>
namespace doodle::maya_plug {
class doodle_bind_node : public MPxNode {
 public:
  static MTypeId doodle_id;
  static constexpr auto node_type = MPxNode::Type::kDependNode;
  static void* creator();
  static MStatus initialize();

  static constexpr std::string_view node_name{"doodle_bind_node"};

 private:
  static MObject M_Head;        // 头部模型
  static MObject M_Brow;        // 眉毛模型
  static MObject L_Cornea;      // 左角膜模型
  static MObject L_Eyeball;     // 左眼球模型
  static MObject L_Iris;        // 左虹膜模型
  static MObject L_Pupil;       // 左瞳孔模型
  static MObject L_Highlight;   // 左高光模型
  static MObject R_Eyeball;     // 右眼球模型
  static MObject R_Cornea;      // 右角膜模型
  static MObject R_Iris;        // 右虹膜模型
  static MObject R_Pupil;       // 右瞳孔模型
  static MObject R_Highlight;   // 右高光模型
  static MObject M_UpEyelash;   // 上睫毛模型
  static MObject M_LoEyelash;   // 下睫毛模型
  static MObject M_Eyelash;     // 睫毛模型
  static MObject M_Tearduct;    // 泪腺模型
  static MObject M_EyelidLine;  // 眼睑模型
  static MObject M_UpTeeth;     // 上牙齿模型
  static MObject M_LoTeeth;     // 下牙齿模型
  static MObject M_UpGum;       // 上牙龈模型
  static MObject M_LoGum;       // 下牙龈模型
  static MObject M_Tongue;      // 舌头模型
  static MObject M_Teeth;       // 牙齿模型
  static MObject M_Gum;         // 牙龈模型
  static MObject M_Saliva;      // 唾液模型
  static MObject M_MustacheA;   // 上胡子模型
  static MObject M_MustacheB;   // 下胡子模型
  static MObject M_Hair;        // 头发模型
  static MObject M_BrowEnd1_locPoint;
  static MObject L_BrowEnd3_locPoint;
  static MObject M_BrowMidUp_locPoint;
  static MObject M_BrowMidLow_locPoint;
  static MObject M_NoseBridge_locPoint;
  static MObject M_NoseUpper_locPoint;
  static MObject M_NoseTip_locPoint;
  static MObject M_NoseLower_locPoint;
  static MObject L_Nostril_locPoint;
  static MObject L_NoseWingA_locPoint;
  static MObject L_NoseUpper_locPoint;
  static MObject L_NoseWingB_locPoint;
  static MObject L_Nasolabial_locPoint;
  static MObject M_Chin_locPoint;
  static MObject L_NoseBridge_locPoint;
  static MObject L_UpperCheekA_locPoint;
  static MObject L_UpperCheekB_locPoint;
  static MObject L_UpperCheekC_locPoint;
  static MObject L_EyeCornerOut_locPoint;
  static MObject L_BrowEnd4_locPoint;
  static MObject L_CheekEndB_locPoint;
  static MObject L_CheekEndA_locPoint;
  static MObject M_JawA_locPoint;
  static MObject L_JawD_locPoint;
  static MObject M_NeckStartA_locPoint;
  static MObject L_NeckStartD_locPoint;
  static MObject M_NeckA_locPoint;
  static MObject L_NeckD_locPoint;
  static MObject M_UpLiplnnerEndA_locPoint;
  static MObject L_UpLiplnnerEndB_locPoint;
  static MObject L_LipCornerlnnerEnd_locPoint;
  static MObject L_LoLiplnnerEndB_locPoint;
  static MObject M_LoLipInnerEndA_locPoint;
  static MObject M_MouthInUpEndA_locPoint;
  static MObject M_MouthlnUpEndB_locPoint;
  static MObject M_MouthlnCornerEnd_locPoint;
  static MObject M_MouthInLoEndB_locPoint;
  static MObject M_MouthInLoEndA_locPoint;
  static MObject L_UplidEdge_A_strCurve;
  static MObject L_LoidEdge_A_strCurve;
  static MObject L_UpLidRing_A_strCurve;
  static MObject L_LoLidRing_A_strCurve;
  static MObject L_UpLidinner_A_strCurve;
  static MObject L_LoLidlnner_A_strCurve;
  static MObject LUpLidEnd_A_strCurve;
  static MObject LoLidEnd_A_strCurve;
  static MObject L_BrowLow_strCurve;
  static MObject L_BrowUp_strCurve;
  static MObject L_Nostril_strCurve;
  static MObject M_UpLipEdge_strCurve;
  static MObject M_LowLipEdge_strCurve;
  static MObject M_UpLiplnner_strCurve;
  static MObject M_LowLiplnner_strCurve;
  static MObject M_BrowEnd_strCurve;
  static MObject M_NoseBridge_strCurve;
  static MObject M_NoseBottom_strCurve;
  static MObject L_NoseWingA_strCurve;
  static MObject L_NoseWingB_strCurve;
  static MObject L_NasolabialFoldA_strCurve;
  static MObject L_NasolabialFoldB_strCurve;
  static MObject L_Cheek_strCurve;
  static MObject L_Jaw_strCurve;
  static MObject L_NeckStartA_strCurve;
  static MObject L_NeckEndA_strCurve;
  static MObject L_NeckStartB_strCurve;
  static MObject L_NeckEndB_strCurve;
  static MObject L_EarFrontA_strCurve;
  static MObject L_EarFrontB_strCurve;
  static MObject L_EarBackA_strCurve;
  static MObject L_EarBackB_strCurve;
  static MObject R_BrowEnd3_locPoint;
  static MObject R_Nostril_locPoint;
  static MObject R_NoseWingA_locPoint;
  static MObject R_NoseUpper_locPoint;
  static MObject R_NoseWingB_locPoint;
  static MObject R_Nasolabial_locPoint;
  static MObject R_NoseBridge_locPoint;
  static MObject R_UpperCheekA_locPoint;
  static MObject R_UpperCheekB_locPoint;
  static MObject R_UpperCheekC_locPoint;
  static MObject R_EyeCornerOut_locPoint;
  static MObject R_BrowEnd4_locPoint;
  static MObject R_CheekEndB_locPoint;
  static MObject R_CheekEndA_locPoint;
  static MObject R_NeckD_locPoint;
  static MObject R_UpLiplnnerEndB_locPoint;
  static MObject R_LipCornerlnnerEnd_locPoint;
  static MObject R_LoLipInnerEndB_locPoint;
  static MObject R_MouthInUpEndB_locPoint;
  static MObject R_MouthlnCornerEnd_locPoint;
  static MObject R_MouthInLoEndB_locPoint;
  static MObject R_UpLidEdge_A_strCurve;
  static MObject R_LoLidEdge_A_strCurve;
  static MObject R_UplLidRing_A_strCurve;
  static MObject R_LoLidRing_A_strCurve;
  static MObject R_UpLidinner_A_strCurve;
  static MObject R_LoLidInner_A_strCurve;
  static MObject R_UpLidEnd_A_strCurve;
  static MObject R_LoLidEnd_A_strCurve;
  static MObject R_BrowLow_strCurve;
  static MObject R_BrowUp_strCurve;
  static MObject R_BrowEnd_strCurve;
  static MObject R_NoseWingA_strCurve;
  static MObject R_NoseWingB_strCurve;
  static MObject R_Nostril_strCurve;
  static MObject R_NasolabialFoldA_strCurve;
  static MObject R_NasolabialFoldB_strCurve;
  static MObject R_Cheek_strCurve;
  static MObject R_Jaw_strCurve;
  static MObject R_NeckStartA_strCurve;
  static MObject R_NeckEndB_strCurve;
  static MObject R_EarFrontA_strCurve;
  static MObject R_EarFrontB_strCurve;
  static MObject R_EarBackA_strCurve;
  static MObject R_EarBackB_strCurve;
};
}  // namespace doodle::maya_plug