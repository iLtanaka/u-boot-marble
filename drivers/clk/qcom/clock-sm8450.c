// SPDX-License-Identifier: BSD-3-Clause
/*
 * Clock drivers for Qualcomm SM8450 / SM8475
 *
 * (C) Copyright 2026 Tatsuki Tanaka <tatsuki@tanakatatsuki.dev>
 * Based on clock-sm8550.c / Linux gcc-sm8450.c
 */

#include <clk-uclass.h>
#include <dm.h>
#include <linux/delay.h>
#include <errno.h>
#include <asm/io.h>
#include <linux/bug.h>
#include <linux/bitops.h>
#include <dt-bindings/clock/qcom,gcc-sm8450.h>

#include "clock-qcom.h"

#define TCXO_RATE	38400000
#define TCXO_DIV2_RATE	(TCXO_RATE / 2)

static const struct freq_tbl ftbl_gcc_qupv3_wrap0_s7_clk_src[] = {
	F(7372800, CFG_CLK_SRC_GPLL0_EVEN, 1, 384, 15625),
	F(14745600, CFG_CLK_SRC_GPLL0_EVEN, 1, 768, 15625),
	F(19200000, CFG_CLK_SRC_CXO, 1, 0, 0),
	F(29491200, CFG_CLK_SRC_GPLL0_EVEN, 1, 1536, 15625),
	F(32000000, CFG_CLK_SRC_GPLL0_EVEN, 1, 8, 75),
	F(48000000, CFG_CLK_SRC_GPLL0_EVEN, 1, 4, 25),
	F(64000000, CFG_CLK_SRC_GPLL0_EVEN, 1, 16, 75),
	F(75000000, CFG_CLK_SRC_GPLL0_EVEN, 4, 0, 0),
	F(80000000, CFG_CLK_SRC_GPLL0_EVEN, 1, 4, 15),
	F(96000000, CFG_CLK_SRC_GPLL0_EVEN, 1, 8, 25),
	F(100000000, CFG_CLK_SRC_GPLL0, 6, 0, 0),
	{ }
};

static const struct freq_tbl ftbl_gcc_sdcc2_apps_clk_src[] = {
	F(400000, CFG_CLK_SRC_CXO, 12, 1, 4),
	F(25000000, CFG_CLK_SRC_GPLL0_EVEN, 12, 0, 0),
	F(37500000, CFG_CLK_SRC_GPLL0_EVEN, 8, 0, 0),
	F(50000000, CFG_CLK_SRC_GPLL0_EVEN, 6, 0, 0),
	F(100000000, CFG_CLK_SRC_GPLL0_EVEN, 3, 0, 0),
	{ }
};

static const struct freq_tbl ftbl_gcc_usb30_prim_master_clk_src[] = {
	F(66666667, CFG_CLK_SRC_GPLL0_EVEN, 4.5, 0, 0),
	F(133333333, CFG_CLK_SRC_GPLL0, 4.5, 0, 0),
	F(200000000, CFG_CLK_SRC_GPLL0, 3, 0, 0),
	F(240000000, CFG_CLK_SRC_GPLL0, 2.5, 0, 0),
	{ }
};

static const struct freq_tbl ftbl_gcc_pcie_0_aux_clk_src[] = {
	F(19200000, CFG_CLK_SRC_CXO, 1, 0, 0),
	{ }
};

static const struct freq_tbl ftbl_gcc_pcie_0_phy_rchng_clk_src[] = {
	F(100000000, CFG_CLK_SRC_GPLL0_EVEN, 3, 0, 0),
	{ }
};

static ulong sm8450_set_rate(struct clk *clk, ulong rate)
{
	struct msm_clk_priv *priv = dev_get_priv(clk->dev);
	const struct freq_tbl *freq;

	switch (clk->id) {
	case GCC_QUPV3_WRAP0_S7_CLK: /* UART7 */
		freq = qcom_find_freq(ftbl_gcc_qupv3_wrap0_s7_clk_src, rate);
		clk_rcg_set_rate_mnd(priv->base, 0x27880,
				     freq->pre_div, freq->m, freq->n, freq->src, 16);
		return freq->freq;
	case GCC_SDCC2_APPS_CLK:
		freq = qcom_find_freq(ftbl_gcc_sdcc2_apps_clk_src, rate);
		clk_rcg_set_rate_mnd(priv->base, 0x24014,
				     freq->pre_div, freq->m, freq->n, freq->src, 8);
		return freq->freq;
	case GCC_USB30_PRIM_MASTER_CLK:
		freq = qcom_find_freq(ftbl_gcc_usb30_prim_master_clk_src, rate);
		clk_rcg_set_rate_mnd(priv->base, 0x49028,
				     freq->pre_div, freq->m, freq->n, freq->src, 8);
		return freq->freq;
	case GCC_USB30_PRIM_MOCK_UTMI_CLK:
		clk_rcg_set_rate(priv->base, 0x49040, 0, 0);
		return TCXO_DIV2_RATE;
	case GCC_USB3_PRIM_PHY_AUX_CLK:
	case GCC_USB3_PRIM_PHY_AUX_CLK_SRC:
		clk_rcg_set_rate(priv->base, 0x4906c, 0, 0);
		return TCXO_DIV2_RATE;
	case GCC_PCIE_0_AUX_CLK:
		freq = qcom_find_freq(ftbl_gcc_pcie_0_aux_clk_src, rate);
		clk_rcg_set_rate_mnd(priv->base, 0x7c074,
				     freq->pre_div, freq->m, freq->n, freq->src, 16);
		return freq->freq;
	case GCC_PCIE_1_AUX_CLK:
		freq = qcom_find_freq(ftbl_gcc_pcie_0_aux_clk_src, rate);
		clk_rcg_set_rate_mnd(priv->base, 0x9e07c,
				     freq->pre_div, freq->m, freq->n, freq->src, 16);
		return freq->freq;
	case GCC_PCIE_0_PHY_RCHNG_CLK:
		freq = qcom_find_freq(ftbl_gcc_pcie_0_phy_rchng_clk_src, rate);
		clk_rcg_set_rate(priv->base, 0x7c058, freq->pre_div, freq->src);
		return freq->freq;
	case GCC_PCIE_1_PHY_RCHNG_CLK:
		freq = qcom_find_freq(ftbl_gcc_pcie_0_phy_rchng_clk_src, rate);
		clk_rcg_set_rate(priv->base, 0x9e060, freq->pre_div, freq->src);
		return freq->freq;
	default:
		return 0;
	}
}

static const struct gate_clk sm8450_clks[] = {
	GATE_CLK(GCC_AGGRE_NOC_PCIE_0_AXI_CLK,		0x62000, BIT(12)),
	GATE_CLK(GCC_AGGRE_NOC_PCIE_1_AXI_CLK,		0x62000, BIT(13)),
	GATE_CLK(GCC_AGGRE_UFS_PHY_AXI_CLK,		0x870d4, BIT(0)),
	GATE_CLK(GCC_AGGRE_UFS_PHY_AXI_HW_CTL_CLK,	0x870d4, BIT(1)),
	GATE_CLK(GCC_AGGRE_USB3_PRIM_AXI_CLK,		0x49088, BIT(0)),
	GATE_CLK(GCC_BOOT_ROM_AHB_CLK,			0x62000, BIT(10)),
	GATE_CLK(GCC_CAMERA_AHB_CLK,			0x36004, BIT(0)),
	GATE_CLK(GCC_CAMERA_HF_AXI_CLK,			0x36010, BIT(0)),
	GATE_CLK(GCC_CAMERA_SF_AXI_CLK,			0x36018, BIT(0)),
	GATE_CLK(GCC_CAMERA_XO_CLK,			0x36020, BIT(0)),
	GATE_CLK(GCC_CFG_NOC_PCIE_ANOC_AHB_CLK,		0x62000, BIT(20)),
	GATE_CLK(GCC_CFG_NOC_USB3_PRIM_AXI_CLK,		0x49084, BIT(0)),
	GATE_CLK(GCC_DDRSS_GPU_AXI_CLK,			0x81154, BIT(0)),
	GATE_CLK(GCC_DDRSS_PCIE_SF_TBU_CLK,		0x62000, BIT(19)),
	GATE_CLK(GCC_DISP_AHB_CLK,			0x37004, BIT(0)),
	GATE_CLK(GCC_DISP_HF_AXI_CLK,			0x37010, BIT(0)),
	GATE_CLK(GCC_DISP_SF_AXI_CLK,			0x37018, BIT(0)),
	GATE_CLK(GCC_DISP_XO_CLK,			0x3701c, BIT(0)),
	GATE_CLK(GCC_EUSB3_0_CLKREF_EN,			0x9c008, BIT(0)),
	GATE_CLK(GCC_GPU_CFG_AHB_CLK,			0x81004, BIT(0)),
	GATE_CLK(GCC_PCIE_0_AUX_CLK,			0x7c008, BIT(0)),
	GATE_CLK(GCC_PCIE_0_CFG_AHB_CLK,		0x7c004, BIT(0)),
	GATE_CLK(GCC_PCIE_0_CLKREF_EN,			0x9c004, BIT(0)),
	GATE_CLK(GCC_PCIE_0_MSTR_AXI_CLK,		0x7c014, BIT(0)),
	GATE_CLK(GCC_PCIE_0_PHY_RCHNG_CLK,		0x7c010, BIT(0)),
	GATE_CLK(GCC_PCIE_0_PIPE_CLK,			0x7c00c, BIT(0)),
	GATE_CLK(GCC_PCIE_0_SLV_AXI_CLK,		0x7c018, BIT(0)),
	GATE_CLK(GCC_PCIE_0_SLV_Q2A_AXI_CLK,		0x7c024, BIT(0)),
	GATE_CLK(GCC_PCIE_1_AUX_CLK,			0x9e008, BIT(0)),
	GATE_CLK(GCC_PCIE_1_CFG_AHB_CLK,		0x9e004, BIT(0)),
	GATE_CLK(GCC_PCIE_1_CLKREF_EN,			0x9c00c, BIT(0)),
	GATE_CLK(GCC_PCIE_1_MSTR_AXI_CLK,		0x9e014, BIT(0)),
	GATE_CLK(GCC_PCIE_1_PHY_AUX_CLK,		0x9e028, BIT(0)),
	GATE_CLK(GCC_PCIE_1_PHY_RCHNG_CLK,		0x9e010, BIT(0)),
	GATE_CLK(GCC_PCIE_1_PIPE_CLK,			0x9e00c, BIT(0)),
	GATE_CLK(GCC_PCIE_1_SLV_AXI_CLK,		0x9e018, BIT(0)),
	GATE_CLK(GCC_PCIE_1_SLV_Q2A_AXI_CLK,		0x9e024, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP0_CORE_2X_CLK,		0x27008, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP0_CORE_CLK,		0x2700c, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP0_S0_CLK,		0x27010, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP0_S1_CLK,		0x27144, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP0_S2_CLK,		0x27278, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP0_S3_CLK,		0x273ac, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP0_S4_CLK,		0x274e0, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP0_S5_CLK,		0x27614, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP0_S6_CLK,		0x27748, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP0_S7_CLK,		0x2787c, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP1_CORE_2X_CLK,		0x28008, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP1_CORE_CLK,		0x2800c, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP1_S0_CLK,		0x28010, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP1_S1_CLK,		0x28144, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP1_S2_CLK,		0x28278, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP1_S3_CLK,		0x283ac, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP1_S4_CLK,		0x284e0, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP1_S5_CLK,		0x28614, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP1_S6_CLK,		0x28748, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP2_CORE_2X_CLK,		0x2e008, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP2_CORE_CLK,		0x2e00c, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP2_S0_CLK,		0x2e010, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP2_S1_CLK,		0x2e144, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP2_S2_CLK,		0x2e278, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP2_S3_CLK,		0x2e3ac, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP2_S4_CLK,		0x2e4e0, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP2_S5_CLK,		0x2e614, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP2_S6_CLK,		0x2e748, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP_0_M_AHB_CLK,		0x27004, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP_0_S_AHB_CLK,		0x27014, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP_1_M_AHB_CLK,		0x28004, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP_1_S_AHB_CLK,		0x28014, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP_2_M_AHB_CLK,		0x2e004, BIT(0)),
	GATE_CLK(GCC_QUPV3_WRAP_2_S_AHB_CLK,		0x2e014, BIT(0)),
	GATE_CLK(GCC_SDCC2_AHB_CLK,			0x24010, BIT(0)),
	GATE_CLK(GCC_SDCC2_APPS_CLK,			0x24004, BIT(0)),
	GATE_CLK(GCC_UFS_0_CLKREF_EN,			0x9c000, BIT(0)),
	GATE_CLK(GCC_UFS_PHY_AHB_CLK,			0x87020, BIT(0)),
	GATE_CLK(GCC_UFS_PHY_AXI_CLK,			0x87018, BIT(0)),
	GATE_CLK(GCC_UFS_PHY_AXI_HW_CTL_CLK,		0x87018, BIT(1)),
	GATE_CLK(GCC_UFS_PHY_ICE_CORE_CLK,		0x8706c, BIT(0)),
	GATE_CLK(GCC_UFS_PHY_ICE_CORE_HW_CTL_CLK,	0x8706c, BIT(1)),
	GATE_CLK(GCC_UFS_PHY_PHY_AUX_CLK,		0x870a4, BIT(0)),
	GATE_CLK(GCC_UFS_PHY_PHY_AUX_HW_CTL_CLK,	0x870a4, BIT(1)),
	GATE_CLK(GCC_UFS_PHY_RX_SYMBOL_0_CLK,		0x87028, BIT(0)),
	GATE_CLK(GCC_UFS_PHY_RX_SYMBOL_1_CLK,		0x870c0, BIT(0)),
	GATE_CLK(GCC_UFS_PHY_TX_SYMBOL_0_CLK,		0x87024, BIT(0)),
	GATE_CLK(GCC_UFS_PHY_UNIPRO_CORE_CLK,		0x87064, BIT(0)),
	GATE_CLK(GCC_UFS_PHY_UNIPRO_CORE_HW_CTL_CLK,	0x87064, BIT(1)),
	GATE_CLK(GCC_USB30_PRIM_MASTER_CLK,		0x49018, BIT(0)),
	GATE_CLK(GCC_USB30_PRIM_MOCK_UTMI_CLK,		0x49024, BIT(0)),
	GATE_CLK(GCC_USB30_PRIM_SLEEP_CLK,		0x49020, BIT(0)),
	GATE_CLK(GCC_USB3_0_CLKREF_EN,			0x9c010, BIT(0)),
	GATE_CLK(GCC_USB3_PRIM_PHY_AUX_CLK,		0x4905c, BIT(0)),
	GATE_CLK(GCC_USB3_PRIM_PHY_COM_AUX_CLK,		0x49060, BIT(0)),
	GATE_CLK(GCC_USB3_PRIM_PHY_PIPE_CLK,		0x49064, BIT(0)),
	GATE_CLK(GCC_VIDEO_AHB_CLK,			0x42004, BIT(0)),
	GATE_CLK(GCC_VIDEO_AXI0_CLK,			0x42018, BIT(0)),
	GATE_CLK(GCC_VIDEO_AXI1_CLK,			0x42024, BIT(0)),
	GATE_CLK(GCC_VIDEO_XO_CLK,			0x42028, BIT(0)),
};

static int sm8450_enable(struct clk *clk)
{
	struct msm_clk_priv *priv = dev_get_priv(clk->dev);

	switch (clk->id) {
	case GCC_USB30_PRIM_MASTER_CLK:
		qcom_gate_clk_en(priv, GCC_USB3_PRIM_PHY_AUX_CLK);
		qcom_gate_clk_en(priv, GCC_USB3_PRIM_PHY_COM_AUX_CLK);
		break;
	case GCC_PCIE_0_PIPE_CLK:
		clk_phy_mux_enable(priv->base, 0x7c070, true);
		break;
	case GCC_PCIE_1_PIPE_CLK:
		clk_phy_mux_enable(priv->base, 0x9e078, true);
		break;
	}

	qcom_gate_clk_en(priv, clk->id);

	return 0;
}

static const struct qcom_reset_map sm8450_gcc_resets[] = {
	[GCC_CAMERA_BCR] = { 0x36000 },
	[GCC_DISPLAY_BCR] = { 0x37000 },
	[GCC_GPU_BCR] = { 0x81000 },
	[GCC_PCIE_0_BCR] = { 0x7b000 },
	[GCC_PCIE_0_LINK_DOWN_BCR] = { 0x7c014 },
	[GCC_PCIE_0_NOCSR_COM_PHY_BCR] = { 0x7c020 },
	[GCC_PCIE_0_PHY_BCR] = { 0x7c01c },
	[GCC_PCIE_0_PHY_NOCSR_COM_PHY_BCR] = { 0x7c028 },
	[GCC_PCIE_1_BCR] = { 0x9d000 },
	[GCC_PCIE_1_LINK_DOWN_BCR] = { 0x9e014 },
	[GCC_PCIE_1_NOCSR_COM_PHY_BCR] = { 0x9e020 },
	[GCC_PCIE_1_PHY_BCR] = { 0x9e01c },
	[GCC_PCIE_1_PHY_NOCSR_COM_PHY_BCR] = { 0x9e000 },
	[GCC_PCIE_PHY_BCR] = { 0x7f000 },
	[GCC_PCIE_PHY_CFG_AHB_BCR] = { 0x7f00c },
	[GCC_PCIE_PHY_COM_BCR] = { 0x7f010 },
	[GCC_PDM_BCR] = { 0x43000 },
	[GCC_QUPV3_WRAPPER_0_BCR] = { 0x27000 },
	[GCC_QUPV3_WRAPPER_1_BCR] = { 0x28000 },
	[GCC_QUPV3_WRAPPER_2_BCR] = { 0x2e000 },
	[GCC_QUSB2PHY_PRIM_BCR] = { 0x22000 },
	[GCC_QUSB2PHY_SEC_BCR] = { 0x22004 },
	[GCC_SDCC2_BCR] = { 0x24000 },
	[GCC_SDCC4_BCR] = { 0x26000 },
	[GCC_UFS_PHY_BCR] = { 0x87000 },
	[GCC_USB30_PRIM_BCR] = { 0x49000 },
	[GCC_USB3_DP_PHY_PRIM_BCR] = { 0x60008 },
	[GCC_USB3_DP_PHY_SEC_BCR] = { 0x60014 },
	[GCC_USB3_PHY_PRIM_BCR] = { 0x60000 },
	[GCC_USB3_PHY_SEC_BCR] = { 0x6000c },
	[GCC_USB3PHY_PHY_PRIM_BCR] = { 0x60004 },
	[GCC_USB3PHY_PHY_SEC_BCR] = { 0x60010 },
	[GCC_USB_PHY_CFG_AHB2PHY_BCR] = { 0x7a000 },
	[GCC_VIDEO_AXI0_CLK_ARES] = { 0x42018, 2 },
	[GCC_VIDEO_AXI1_CLK_ARES] = { 0x42024, 2 },
	[GCC_VIDEO_BCR] = { 0x42000 },
};

static const struct qcom_power_map sm8450_gdscs[] = {
	[PCIE_0_GDSC] = { 0x7b004 },
	[PCIE_1_GDSC] = { 0x9d004 },
	[UFS_PHY_GDSC] = { 0x87004 },
	[USB30_PRIM_GDSC] = { 0x49004 },
};

static struct msm_clk_data sm8450_gcc_data = {
	.resets = sm8450_gcc_resets,
	.num_resets = ARRAY_SIZE(sm8450_gcc_resets),
	.clks = sm8450_clks,
	.num_clks = ARRAY_SIZE(sm8450_clks),
	.power_domains = sm8450_gdscs,
	.num_power_domains = ARRAY_SIZE(sm8450_gdscs),

	.enable = sm8450_enable,
	.set_rate = sm8450_set_rate,
};

static const struct udevice_id gcc_sm8450_of_match[] = {
	{
		.compatible = "qcom,gcc-sm8450",
		.data = (ulong)&sm8450_gcc_data,
	},
	{
		.compatible = "qcom,sm8475-gcc",
		.data = (ulong)&sm8450_gcc_data,
	},
	{ }
};

U_BOOT_DRIVER(gcc_sm8450) = {
	.name		= "gcc_sm8450",
	.id		= UCLASS_NOP,
	.of_match	= gcc_sm8450_of_match,
	.bind		= qcom_cc_bind,
	.flags		= DM_FLAG_PRE_RELOC | DM_FLAG_DEFAULT_PD_CTRL_OFF,
};
