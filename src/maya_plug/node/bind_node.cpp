//
// Created by TD on 25-7-21.
//

#include "bind_node.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MFnMessageAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MStatus.h>
#include <maya/MTypeId.h>
namespace doodle::maya_plug {

MTypeId doodle_bind_node::doodle_id = MTypeId{0x00000002};

MObject doodle_bind_node::M_Head;
MObject doodle_bind_node::M_Brow;
MObject doodle_bind_node::L_Cornea;
MObject doodle_bind_node::L_Eyeball;
MObject doodle_bind_node::L_Iris;
MObject doodle_bind_node::L_Pupil;
MObject doodle_bind_node::L_Highlight;
MObject doodle_bind_node::R_Eyeball;
MObject doodle_bind_node::R_Cornea;
MObject doodle_bind_node::R_Iris;
MObject doodle_bind_node::R_Pupil;
MObject doodle_bind_node::R_Highlight;
MObject doodle_bind_node::M_UpEyelash;
MObject doodle_bind_node::M_LoEyelash;
MObject doodle_bind_node::M_Eyelash;
MObject doodle_bind_node::M_Tearduct;
MObject doodle_bind_node::M_EyelidLine;
MObject doodle_bind_node::M_UpTeeth;
MObject doodle_bind_node::M_LoTeeth;
MObject doodle_bind_node::M_UpGum;
MObject doodle_bind_node::M_LoGum;
MObject doodle_bind_node::M_Tongue;
MObject doodle_bind_node::M_Teeth;
MObject doodle_bind_node::M_Gum;
MObject doodle_bind_node::M_Saliva;
MObject doodle_bind_node::M_MustacheA;
MObject doodle_bind_node::M_MustacheB;
MObject doodle_bind_node::M_Hair;
MObject doodle_bind_node::M_BrowEnd1_locPoint;
MObject doodle_bind_node::L_BrowEnd3_locPoint;
MObject doodle_bind_node::M_BrowMidUp_locPoint;
MObject doodle_bind_node::M_BrowMidLow_locPoint;
MObject doodle_bind_node::M_NoseBridge_locPoint;
MObject doodle_bind_node::M_NoseUpper_locPoint;
MObject doodle_bind_node::M_NoseTip_locPoint;
MObject doodle_bind_node::M_NoseLower_locPoint;
MObject doodle_bind_node::L_Nostril_locPoint;
MObject doodle_bind_node::L_NoseWingA_locPoint;
MObject doodle_bind_node::L_NoseUpper_locPoint;
MObject doodle_bind_node::L_NoseWingB_locPoint;
MObject doodle_bind_node::L_Nasolabial_locPoint;
MObject doodle_bind_node::M_Chin_locPoint;
MObject doodle_bind_node::L_NoseBridge_locPoint;
MObject doodle_bind_node::L_UpperCheekA_locPoint;
MObject doodle_bind_node::L_UpperCheekB_locPoint;
MObject doodle_bind_node::L_UpperCheekC_locPoint;
MObject doodle_bind_node::L_EyeCornerOut_locPoint;
MObject doodle_bind_node::L_BrowEnd4_locPoint;
MObject doodle_bind_node::L_CheekEndB_locPoint;
MObject doodle_bind_node::L_CheekEndA_locPoint;
MObject doodle_bind_node::M_JawA_locPoint;
MObject doodle_bind_node::L_JawD_locPoint;
MObject doodle_bind_node::M_NeckStartA_locPoint;
MObject doodle_bind_node::L_NeckStartD_locPoint;
MObject doodle_bind_node::M_NeckA_locPoint;
MObject doodle_bind_node::L_NeckD_locPoint;
MObject doodle_bind_node::M_UpLiplnnerEndA_locPoint;
MObject doodle_bind_node::L_UpLiplnnerEndB_locPoint;
MObject doodle_bind_node::L_LipCornerlnnerEnd_locPoint;
MObject doodle_bind_node::L_LoLiplnnerEndB_locPoint;
MObject doodle_bind_node::M_LoLipInnerEndA_locPoint;
MObject doodle_bind_node::M_MouthInUpEndA_locPoint;
MObject doodle_bind_node::M_MouthlnUpEndB_locPoint;
MObject doodle_bind_node::M_MouthlnCornerEnd_locPoint;
MObject doodle_bind_node::M_MouthInLoEndB_locPoint;
MObject doodle_bind_node::M_MouthInLoEndA_locPoint;
MObject doodle_bind_node::L_UplidEdge_A_strCurve;
MObject doodle_bind_node::L_LoidEdge_A_strCurve;
MObject doodle_bind_node::L_UpLidRing_A_strCurve;
MObject doodle_bind_node::L_LoLidRing_A_strCurve;
MObject doodle_bind_node::L_UpLidinner_A_strCurve;
MObject doodle_bind_node::L_LoLidlnner_A_strCurve;
MObject doodle_bind_node::LUpLidEnd_A_strCurve;
MObject doodle_bind_node::LoLidEnd_A_strCurve;
MObject doodle_bind_node::L_BrowLow_strCurve;
MObject doodle_bind_node::L_BrowUp_strCurve;
MObject doodle_bind_node::L_Nostril_strCurve;
MObject doodle_bind_node::M_UpLipEdge_strCurve;
MObject doodle_bind_node::M_LowLipEdge_strCurve;
MObject doodle_bind_node::M_UpLiplnner_strCurve;
MObject doodle_bind_node::M_LowLiplnner_strCurve;
MObject doodle_bind_node::M_BrowEnd_strCurve;
MObject doodle_bind_node::M_NoseBridge_strCurve;
MObject doodle_bind_node::M_NoseBottom_strCurve;
MObject doodle_bind_node::L_NoseWingA_strCurve;
MObject doodle_bind_node::L_NoseWingB_strCurve;
MObject doodle_bind_node::L_NasolabialFoldA_strCurve;
MObject doodle_bind_node::L_NasolabialFoldB_strCurve;
MObject doodle_bind_node::L_Cheek_strCurve;
MObject doodle_bind_node::L_Jaw_strCurve;
MObject doodle_bind_node::L_NeckStartA_strCurve;
MObject doodle_bind_node::L_NeckEndA_strCurve;
MObject doodle_bind_node::L_NeckStartB_strCurve;
MObject doodle_bind_node::L_NeckEndB_strCurve;
MObject doodle_bind_node::L_EarFrontA_strCurve;
MObject doodle_bind_node::L_EarFrontB_strCurve;
MObject doodle_bind_node::L_EarBackA_strCurve;
MObject doodle_bind_node::L_EarBackB_strCurve;
MObject doodle_bind_node::R_BrowEnd3_locPoint;
MObject doodle_bind_node::R_Nostril_locPoint;
MObject doodle_bind_node::R_NoseWingA_locPoint;
MObject doodle_bind_node::R_NoseUpper_locPoint;
MObject doodle_bind_node::R_NoseWingB_locPoint;
MObject doodle_bind_node::R_Nasolabial_locPoint;
MObject doodle_bind_node::R_NoseBridge_locPoint;
MObject doodle_bind_node::R_UpperCheekA_locPoint;
MObject doodle_bind_node::R_UpperCheekB_locPoint;
MObject doodle_bind_node::R_UpperCheekC_locPoint;
MObject doodle_bind_node::R_EyeCornerOut_locPoint;
MObject doodle_bind_node::R_BrowEnd4_locPoint;
MObject doodle_bind_node::R_CheekEndB_locPoint;
MObject doodle_bind_node::R_CheekEndA_locPoint;
MObject doodle_bind_node::R_NeckD_locPoint;
MObject doodle_bind_node::R_UpLiplnnerEndB_locPoint;
MObject doodle_bind_node::R_LipCornerlnnerEnd_locPoint;
MObject doodle_bind_node::R_LoLipInnerEndB_locPoint;
MObject doodle_bind_node::R_MouthInUpEndB_locPoint;
MObject doodle_bind_node::R_MouthlnCornerEnd_locPoint;
MObject doodle_bind_node::R_MouthInLoEndB_locPoint;
MObject doodle_bind_node::R_UpLidEdge_A_strCurve;
MObject doodle_bind_node::R_LoLidEdge_A_strCurve;
MObject doodle_bind_node::R_UplLidRing_A_strCurve;
MObject doodle_bind_node::R_LoLidRing_A_strCurve;
MObject doodle_bind_node::R_UpLidinner_A_strCurve;
MObject doodle_bind_node::R_LoLidInner_A_strCurve;
MObject doodle_bind_node::R_UpLidEnd_A_strCurve;
MObject doodle_bind_node::R_LoLidEnd_A_strCurve;
MObject doodle_bind_node::R_BrowLow_strCurve;
MObject doodle_bind_node::R_BrowUp_strCurve;
MObject doodle_bind_node::R_BrowEnd_strCurve;
MObject doodle_bind_node::R_NoseWingA_strCurve;
MObject doodle_bind_node::R_NoseWingB_strCurve;
MObject doodle_bind_node::R_Nostril_strCurve;
MObject doodle_bind_node::R_NasolabialFoldA_strCurve;
MObject doodle_bind_node::R_NasolabialFoldB_strCurve;
MObject doodle_bind_node::R_Cheek_strCurve;
MObject doodle_bind_node::R_Jaw_strCurve;
MObject doodle_bind_node::R_NeckStartA_strCurve;
MObject doodle_bind_node::R_NeckEndB_strCurve;
MObject doodle_bind_node::R_EarFrontA_strCurve;
MObject doodle_bind_node::R_EarFrontB_strCurve;
MObject doodle_bind_node::R_EarBackA_strCurve;
MObject doodle_bind_node::R_EarBackB_strCurve;
#define DOODLE_BIND_NODE_ARRT(type, name, short_name)        \
  {                                                          \
    MFnMessageAttribute l_msg_attr{};                        \
    name = l_msg_attr.create(#name, #short_name, &l_status); \
    CHECK_MSTATUS_AND_RETURN_IT(l_status);                   \
    l_msg_attr.setStorable(true);                            \
    l_msg_attr.setWritable(true);                            \
    l_msg_attr.setConnectable(true);                         \
    addAttribute(name);                                      \
  }

MStatus doodle_bind_node::initialize() {
  MStatus l_status{};

  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_Head, m_hd);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_Brow, m_br);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_Cornea, l_crn);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_Eyeball, l_eyb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_Iris, l_irs);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_Pupil, l_pup);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_Highlight, l_hlt);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_Eyeball, r_eyb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_Cornea, r_crn);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_Iris, r_irs);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_Pupil, r_pup);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_Highlight, r_hlt);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_UpEyelash, m_uel);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_LoEyelash, m_lel);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_Eyelash, m_ela);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_Tearduct, m_tdt);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_EyelidLine, m_edl);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_UpTeeth, m_upt);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_LoTeeth, m_lot);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_UpGum, m_upg);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_LoGum, m_log);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_Tongue, m_tng);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_Teeth, m_teh);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_Gum, gumm_gum);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_Saliva, m_slv);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_MustacheA, m_msa);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_MustacheB, m_msb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_Hair, m_har);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_BrowEnd1_locPoint, m_bep);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_BrowEnd3_locPoint, l_bep);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_BrowMidUp_locPoint, m_bmu);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_BrowMidLow_locPoint, m_bml);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_NoseBridge_locPoint, m_nbr);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_NoseUpper_locPoint, m_nsu);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_NoseTip_locPoint, m_ntp);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_NoseLower_locPoint, m_nlr);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_Nostril_locPoint, l_nsl);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_NoseWingA_locPoint, l_nwa);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_NoseUpper_locPoint, l_nsu);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_NoseWingB_locPoint, l_nwb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_Nasolabial_locPoint, l_nlb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_Chin_locPoint, m_chn);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_NoseBridge_locPoint, l_nbr);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_UpperCheekA_locPoint, l_uca);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_UpperCheekB_locPoint, l_ucb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_UpperCheekC_locPoint, l_ucc);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_EyeCornerOut_locPoint, l_eco);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_BrowEnd4_locPoint, l_bep);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_CheekEndB_locPoint, l_ceb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_CheekEndA_locPoint, l_cea);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_JawA_locPoint, m_jwa);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_JawD_locPoint, l_jwd);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_NeckStartA_locPoint, m_nsa);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_NeckStartD_locPoint, l_nsd);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_NeckA_locPoint, m_nka);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_NeckD_locPoint, l_nkd);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_UpLiplnnerEndA_locPoint, m_ula);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_UpLiplnnerEndB_locPoint, l_ulb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_LipCornerlnnerEnd_locPoint, l_lce);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_LoLiplnnerEndB_locPoint, l_llb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_LoLipInnerEndA_locPoint, m_lla);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_MouthInUpEndA_locPoint, m_mua);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_MouthlnUpEndB_locPoint, m_mub);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_MouthlnCornerEnd_locPoint, m_mce);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_MouthInLoEndB_locPoint, m_mlb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_MouthInLoEndA_locPoint, m_mla);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_UplidEdge_A_strCurve, l_ule);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_LoidEdge_A_strCurve, l_lle);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_UpLidRing_A_strCurve, l_ulr);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_LoLidRing_A_strCurve, l_llr);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_UpLidinner_A_strCurve, l_uli);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_LoLidlnner_A_strCurve, l_lli);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, LUpLidEnd_A_strCurve, l_ule);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, LoLidEnd_A_strCurve, l_lle);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_BrowLow_strCurve, l_blw);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_BrowUp_strCurve, l_bup);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_Nostril_strCurve, l_nsl);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_UpLipEdge_strCurve, m_upe);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_LowLipEdge_strCurve, m_lpe);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_UpLiplnner_strCurve, m_upl);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_LowLiplnner_strCurve, m_lpl);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_BrowEnd_strCurve, m_bes);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_NoseBridge_strCurve, m_nbs);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, M_NoseBottom_strCurve, m_nbb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_NoseWingA_strCurve, l_nwa);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_NoseWingB_strCurve, l_nwb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_NasolabialFoldA_strCurve, l_nfa);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_NasolabialFoldB_strCurve, l_nfb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_Cheek_strCurve, l_chk);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_Jaw_strCurve, l_jaw);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_NeckStartA_strCurve, l_nsa);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_NeckEndA_strCurve, l_nea);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_NeckStartB_strCurve, l_nsb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_NeckEndB_strCurve, l_neb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_EarFrontA_strCurve, l_efa);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_EarFrontB_strCurve, l_efb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_EarBackA_strCurve, l_eba);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, L_EarBackB_strCurve, l_ebb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_BrowEnd3_locPoint, r_bep);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_Nostril_locPoint, r_nsl);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_NoseWingA_locPoint, r_nwa);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_NoseUpper_locPoint, r_nsu);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_NoseWingB_locPoint, r_nwb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_Nasolabial_locPoint, r_nlb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_NoseBridge_locPoint, r_nbr);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_UpperCheekA_locPoint, r_uca);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_UpperCheekB_locPoint, r_ucb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_UpperCheekC_locPoint, r_ucc);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_EyeCornerOut_locPoint, r_eco);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_BrowEnd4_locPoint, r_bep);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_CheekEndB_locPoint, r_ceb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_CheekEndA_locPoint, r_cea);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_NeckD_locPoint, r_nkd);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_UpLiplnnerEndB_locPoint, r_ulb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_LipCornerlnnerEnd_locPoint, r_lce);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_LoLipInnerEndB_locPoint, r_llb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_MouthInUpEndB_locPoint, r_mub);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_MouthlnCornerEnd_locPoint, r_mce);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_MouthInLoEndB_locPoint, r_mlb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_UpLidEdge_A_strCurve, r_ule);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_LoLidEdge_A_strCurve, r_lle);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_UplLidRing_A_strCurve, r_ulr);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_LoLidRing_A_strCurve, r_llr);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_UpLidinner_A_strCurve, r_uli);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_LoLidInner_A_strCurve, r_lli);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_UpLidEnd_A_strCurve, r_ule);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_LoLidEnd_A_strCurve, r_lle);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_BrowLow_strCurve, r_blw);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_BrowUp_strCurve, r_bup);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_BrowEnd_strCurve, r_bes);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_NoseWingA_strCurve, r_nwa);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_NoseWingB_strCurve, r_nwb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_Nostril_strCurve, r_nsl);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_NasolabialFoldA_strCurve, r_nfa);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_NasolabialFoldB_strCurve, r_nfb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_Cheek_strCurve, r_chk);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_Jaw_strCurve, r_jaw);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_NeckStartA_strCurve, r_nsa);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_NeckEndB_strCurve, r_neb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_EarFrontA_strCurve, r_efa);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_EarFrontB_strCurve, r_efb);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_EarBackA_strCurve, r_eba);
  DOODLE_BIND_NODE_ARRT(MFnMessageAttribute, R_EarBackB_strCurve, r_ebb);

  return MS::kSuccess;
}
void* doodle_bind_node::creator() {}

}  // namespace doodle::maya_plug
