/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __ASM_ARCH_KS8695_REGS_H
#define __ASM_ARCH_KS8695_REGS_H 1

#define KS8695_SYSTEN_CONFIG		0x00
#define KS8695_SYSTEN_BUS_CLOCK		0x04

/* bus clock definitions*/
#define KS8695_BUS_CLOCK_125MHZ		0x0
#define KS8695_BUS_CLOCK_100MHZ		0x1
#define KS8695_BUS_CLOCK_62MHZ		0x2
#define KS8695_BUS_CLOCK_50MHZ		0x3
#define KS8695_BUS_CLOCK_41MHZ		0x4
#define KS8695_BUS_CLOCK_33MHZ		0x5
#define KS8695_BUS_CLOCK_31MHZ		0x6
#define KS8695_BUS_CLOCK_25MHZ		0x7

/* i/o control registers offset difinitions */
#define KS8695_IO_CTRL0			0x4000
#define KS8695_IO_CTRL1			0x4004
#define KS8695_IO_CTRL2			0x4008
#define KS8695_IO_CTRL3			0x400C

/* memory control registers offset difinitions */
#define KS8695_MEM_CTRL0		0x4010
#define KS8695_MEM_CTRL1		0x4014
#define KS8695_MEM_CTRL2		0x4018
#define KS8695_MEM_CTRL3		0x401C
#define KS8695_MEM_GENERAL		0x4020
#define KS8695_SDRAM_CTRL0		0x4030 
#define KS8695_SDRAM_CTRL1		0x4034
#define KS8695_SDRAM_GENERAL		0x4038
#define KS8695_SDRAM_BUFFER		0x403C
#define KS8695_SDRAM_REFRESH		0x4040    

/* WAN control registers offset difinitions */
#define KS8695_WAN_DMA_TX		0x6000
#define KS8695_WAN_DMA_RX		0x6004
#define KS8695_WAN_DMA_TX_START		0x6008    
#define KS8695_WAN_DMA_RX_START		0x600C    
#define KS8695_WAN_TX_LIST		0x6010
#define KS8695_WAN_RX_LIST		0x6014
#define KS8695_WAN_MAC_LOW		0x6018
#define KS8695_WAN_MAC_HIGH		0x601C
#define KS8695_WAN_MAC_ELOW		0x6080
#define KS8695_WAN_MAC_EHIGH		0x6084

/* LAN control registers offset difinitions */
#define KS8695_LAN_DMA_TX		0x8000
#define KS8695_LAN_DMA_RX		0x8004
#define KS8695_LAN_DMA_TX_START		0x8008    
#define KS8695_LAN_DMA_RX_START		0x800C    
#define KS8695_LAN_TX_LIST		0x8010
#define KS8695_LAN_RX_LIST		0x8014
#define KS8695_LAN_MAC_LOW		0x8018
#define KS8695_LAN_MAC_HIGH		0x801C
#define KS8695_LAN_MAC_ELOW		0X8080
#define KS8695_LAN_MAC_EHIGH		0X8084

/* HPNA control registers offset difinitions */
#define KS8695_HPNA_DMA_TX		0xA000
#define KS8695_HPNA_DMA_RX		0xA004
#define KS8695_HPNA_DMA_TX_START	0xA008    
#define KS8695_HPNA_DMA_RX_START	0xA00C    
#define KS8695_HPNA_TX_LIST		0xA010
#define KS8695_HPNA_RX_LIST		0xA014
#define KS8695_HPNA_MAC_LOW		0xA018
#define KS8695_HPNA_MAC_HIGH		0xA01C
#define KS8695_HPNA_MAC_ELOW		0xA080
#define KS8695_HPNA_MAC_EHIGH		0xA084

/* UART control registers offset difinitions */
#define KS8695_UART_RX_BUFFER		0xE000
#define KS8695_UART_TX_HOLDING		0xE004

#define KS8695_UART_FIFO_CTRL		0xE008
#define KS8695_UART_FIFO_TRIG01		0x00
#define KS8695_UART_FIFO_TRIG04		0x80
#define KS8695_UART_FIFO_TXRST		0x03
#define KS8695_UART_FIFO_RXRST		0x02
#define KS8695_UART_FIFO_FEN		0x01

#define KS8695_UART_LINE_CTRL		0xE00C
#define KS8695_UART_LINEC_BRK		0x40
#define KS8695_UART_LINEC_EPS		0x10
#define KS8695_UART_LINEC_PEN		0x08
#define KS8695_UART_LINEC_STP2		0x04
#define KS8695_UART_LINEC_WLEN8		0x03
#define KS8695_UART_LINEC_WLEN7		0x02
#define KS8695_UART_LINEC_WLEN6		0x01
#define KS8695_UART_LINEC_WLEN5		0x00

#define KS8695_UART_MODEM_CTRL		0xE010
#define KS8695_UART_MODEMC_RTS		0x02
#define KS8695_UART_MODEMC_DTR		0x01

#define KS8695_UART_LINE_STATUS		0xE014
#define KS8695_UART_LINES_TXFE		0x20
#define KS8695_UART_LINES_BE		0x10
#define KS8695_UART_LINES_FE		0x08
#define KS8695_UART_LINES_PE		0x04
#define KS8695_UART_LINES_OE		0x02
#define KS8695_UART_LINES_RXFE		0x01
#define KS8695_UART_LINES_ANY		(KS8695_UART_LINES_OE | \
					 KS8695_UART_LINES_BE | \
					 KS8695_UART_LINES_PE | \
					 KS8695_UART_LINES_FE)

#define KS8695_UART_MODEM_STATUS	0xE018
#define KS8695_UART_MODEM_DCD		0x80
#define KS8695_UART_MODEM_DSR		0x20
#define KS8695_UART_MODEM_CTS		0x10
#define KS8695_UART_MODEM_DDCD		0x08
#define KS8695_UART_MODEM_DDSR		0x02
#define KS8695_UART_MODEM_DCTS		0x01
#define KS8695_UART_MODEM_ANY		0xFF

#define KS8695_UART_DIVISOR		0xE01C
#define KS8695_UART_STATUS		0xE020

/* Interrupt controlller registers offset difinitions */
#define KS8695_INT_CONTL		0xE200
#define KS8695_INT_ENABLE		0xE204
#define KS8695_INT_ENABLE_MODEM		0x0800
#define KS8695_INT_ENABLE_ERR		0x0400
#define KS8695_INT_ENABLE_RX		0x0200
#define KS8695_INT_ENABLE_TX		0x0100
#define	KS8695_INT_UART_MASK		0x0f00

#define KS8695_INT_STATUS		0xE208
#define KS8695_INT_WAN_PRIORITY		0xE20C
#define KS8695_INT_HPNA_PRIORITY	0xE210
#define KS8695_INT_LAN_PRIORITY		0xE214
#define KS8695_INT_TIMER_PRIORITY	0xE218
#define KS8695_INT_UART_PRIORITY	0xE21C
#define KS8695_INT_EXT_PRIORITY		0xE220
#define KS8695_INT_CHAN_PRIORITY	0xE224
#define KS8695_INT_BUSERROR_PRO		0xE228
#define KS8695_INT_MASK_STATUS		0xE22C
#define KS8695_FIQ_PEND_PRIORITY	0xE230
#define KS8695_IRQ_PEND_PRIORITY	0xE234

/* timer registers offset difinitions */
#define KS8695_TIMER_CTRL		0xE400
#define KS8695_TIMER1			0xE404
#define KS8695_TIMER0			0xE408
#define KS8695_TIMER1_PCOUNT		0xE40C
#define KS8695_TIMER0_PCOUNT		0xE410

/* GPIO registers offset difinitions */
#define KS8695_GPIO_MODE		0xE600
#define KS8695_GPIO_CTRL		0xE604
#define KS8695_GPIO_DATA		0xE608

/* SWITCH registers offset difinitions */
#define KS8695_SWITCH_CTRL0		0xE800
#define KS8695_SWITCH_CTRL1		0xE804
#define KS8695_SWITCH_PORT1		0xE808
#define KS8695_SWITCH_PORT2		0xE80C
#define KS8695_SWITCH_PORT3		0xE810
#define KS8695_SWITCH_PORT4		0xE814
#define KS8695_SWITCH_PORT5 		0xE818
#define KS8695_SWITCH_LUE_CTRL		0xE824
#define KS8695_SWITCH_LUE_HIGH		0xE828
#define KS8695_SWITCH_LUE_LOW		0xE82C

/* some differences between the KS8695(X) and KS8695P */
#ifdef CONFIG_PCI
#define KS8695_SWITCH_AUTO0		0xE848
#define KS8695_SWITCH_AUTO1		0xE84C
#define KS8695_SWITCH_ADVANCED		0xE860
#define KS8695_DSCP_HIGH		0xE864
#define KS8695_DSCP_LOW			0xE868
#define KS8695_SWITCH_MAC_HIGH		0xE86C
#define KS8695_SWITCH_MAC_LOW		0xE870
#define KS8695_LAN12_POWERMAGR		0xE874
#define KS8695_LAN34_POWERMAGR		0xE878
#else
#define KS8695_SWITCH_AUTO0		0xE81C
#define KS8695_SWITCH_AUTO1		0xE820
#define KS8695_SWITCH_ADVANCED		0xE830
#define KS8695_DSCP_HIGH		0xE834
#define KS8695_DSCP_LOW			0xE838
#define KS8695_SWITCH_MAC_HIGH		0xE83C
#define KS8695_SWITCH_MAC_LOW		0xE840
#define KS8695_LAN12_POWERMAGR		0xE84C
#define KS8695_LAN34_POWERMAGR		0xE850
#endif

/* miscellaneours registers difinitions */
#define KS8695_MANAGE_COUNTER		0xE844
#define KS8695_MANAGE_DATA		0xE848

#define KS8695_DEVICE_ID		0xEA00
#define KS8695_REVISION_ID		0xEA04

#define KS8695_MISC_CONTROL		0xEA08
#define KS8695_WAN_CONTROL		0xEA0C
#define KS8695_WAN_POWERMAGR		0xEA10
#define KS8695_WAN_PHY_CONTROL		0xEA14
#define KS8695_WAN_PHY_STATUS		0xEA18


/*
 * The following are all new in the KS8695P.
 */
#ifdef CONFIG_PCI

/* most bit definition are same as KS8695, except few new bits */
#define KS8695_SEC0			0xE800
#define KS8695_SEC1			0xE804

/* new bits */
#define	KS8695_SEC0_BACKOFF_EN		0x80000000
#define	KS8695_SEC0_FRAME_LEN_CHECK	0x00020000
#define	KS8695_SEC0_DMA_HALF_DUPLEX	0x00000010

/* new bits */
#define	KS8695_SEC1_NO_IEEE_AN		0x00000800
#define	KS8695_SEC1_TPID_MODE		0x00000400
#define	KS8695_SEC1_NO_TX_8021X_FLOW_CTRL 0x00000080
#define	KS8695_SEC1_NO_RX_8021X_FLOW_CTRL 0x00000040
#define	KS8695_SEC1_HUGE_PACKET		0x00000020
#define	KS8695_SEC1_8021Q_VLAN_EN	0x00000010
#define	KS8695_SEC1_MII_10BT		0x00000002
#define	KS8695_SEC1_NULL_VID		0x00000001

/* Port 1-4 and 5 Configuration Register Set 1 */
#define KS8695_SEP1C1			0xE80C
#define KS8695_SEP2C1			0xE818
#define KS8695_SEP3C1			0xE824
#define KS8695_SEP4C1			0xE830
#define KS8695_SEP5C1			0xE83C

/* Port 1-4 and 5 Configuration Register Set 2 */
#define KS8695_SEP1C2			0xE810
#define KS8695_SEP2C2			0xE81C
#define KS8695_SEP3C2			0xE828
#define KS8695_SEP4C2			0xE834
#define KS8695_SEP5C2			0xE840

#define	KS8695_SEPC2_VLAN_FILTER	0x10000000
#define	KS8695_SEPC2_DISCARD_NON_PVID	0x08000000
#define	KS8695_SEPC2_FORCE_FLOW_CTRL	0x04000000
#define	KS8695_SEPC2_BACK_PRESSURE_EN	0x02000000

#define	KS8695_SEPC2_TX_H_RATECTRL_MASK	0x00FFF000
#define	KS8695_SEPC2_TX_L_RATECTRL_MASK	0x00000FFF

/* Port 1-4 and 5 Configuration Register Set 3 */
#define KS8695_SEP1C3			0xE814
#define KS8695_SEP2C3			0xE820
#define KS8695_SEP3C3			0xE82C
#define KS8695_SEP4C3			0xE838
#define KS8695_SEP5C3			0xE844

#define	KS8695_SEPC3_RX_H_RATECTRL_MASK	0xFFF00000
#define	KS8695_SEPC3_RX_L_RATECTRL_MASK	0x000FFF00
#define	KS8695_SEPC3_RX_DIF_RATECTRL_EN	0x00000080
#define	KS8695_SEPC3_RX_L_RATECTRL_EN	0x00000040
#define	KS8695_SEPC3_RX_H_RATECTRL_EN	0x00000020
#define	KS8695_SEPC3_RX_L_RATEFLOW_EN	0x00000010
#define	KS8695_SEPC3_RX_H_RATEFLOW_EN	0x00000008
#define	KS8695_SEPC3_TX_DIF_RATECTRL_EN	0x00000004
#define	KS8695_SEPC3_TX_L_RATECTRL_EN	0x00000002
#define	KS8695_SEPC3_TX_H_RATECTRL_EN	0x00000001

/* Port auto negotiation related registers */
#define KS8695_SEP12AN			0xE848
#define KS8695_SEP34AN			0xE84C

/* Indirect Access Control register */
#define KS8695_SEIAC			0xE850
#define KS8695_SEIADH2			0xE854
#define KS8695_SEIADH1			0xE858
#define KS8695_SEIADL			0xE85C

#define	KS8695_SEIAC_READ		0x00001000
#define	KS8695_SEIAC_WRITE		0x00000000
#define	KS8695_SEIAC_TAB_STATIC		0x00000000
#define	KS8695_SEIAC_TAB_VLAN		0x00000400
#define	KS8695_SEIAC_TAB_DYNAMIC	0x00000800
#define	KS8695_SEIAC_TAB_MIB		0x00000C00
#define	KS8695_SEIAC_INDEX_MASK		0x000003FF

/* Advanced Feature Control register */
#define KS8695_SEAFC			0xE860
#define KS8695_SEDSCPH			0xE864
#define KS8695_SEDSCPL			0xE868
#define KS8695_SEMAH			0xE86C
#define KS8695_SEMAL			0xE870

/* LAN PHY power management related registers */
#define KS8695_LPPM12			0xE874
#define KS8695_LPPM34			0xE878

/* new bits */
#define	KS8695_LPPM_PHY_LOOPBACK	0x4000
#define	KS8695_LPPM_RMT_LOOPBACK	0x2000
#define	KS8695_LPPM_PHY_ISOLATE		0x1000
#define	KS8695_LPPM_SOFT_RESET		0x0800
#define	KS8695_LPPM_FORCE_LINK		0x0400

/* new bits */
#define	KS8695_LPPM_PHY_LOOPBACK	0x4000
#define	KS8695_LPPM_RMT_LOOPBACK	0x2000
#define	KS8695_LPPM_PHY_ISOLATE		0x1000
#define	KS8695_LPPM_SOFT_RESET		0x0800
#define	KS8695_LPPM_FORCE_LINK		0x0400

/* Digital Testing Status and Control Registers */
#define KS8695_SEDTS			0xE87C
#define KS8695_SEATCS			0xE880

/* new bits for WAN PHY Power mangement register */
#define	KS8695_WPPM_PHY_LOOPBACK	0x00004000
#define	KS8695_WPPM_RMT_LOOPBACK	0x00002000
#define	KS8695_WPPM_PHY_ISOLATION	0x00001000
#define	KS8695_WPPM_FORCE_LINK		0x00000400

#endif /* CONFIG_PCI */

#endif /* __ASM_ARCH_KS8695_REGS_H */
