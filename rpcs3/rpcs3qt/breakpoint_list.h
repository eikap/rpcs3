﻿#pragma once

#include "stdafx.h"
#include "Emu/CPU/CPUDisAsm.h"

#include "breakpoint_handler.h"

#include <QListWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QPointer>

class breakpoint_list : public QListWidget
{
	Q_OBJECT

public:
	breakpoint_list(QWidget* parent, breakpoint_handler* handler);
	void UpdateCPUData(std::weak_ptr<cpu_thread> cpu, std::shared_ptr<CPUDisAsm> disasm);
	void ClearBreakpoints();
	void AddBreakpoint(u32 addr, bs_t<breakpoint_type> type);
	void RemoveBreakpoint(u32 addr);
	void ShowAddBreakpointWindow();

	QColor m_text_color_bp;
	QColor m_color_bp;
Q_SIGNALS:
	void RequestShowAddress(u32 addr);
public Q_SLOTS:
	void HandleBreakpointRequest(u32 addr);
private Q_SLOTS:
	void OnBreakpointListDoubleClicked();
	void OnBreakpointListRightClicked(const QPoint &pos);
	void OnBreakpointListDelete();
	void OnBreakPointListBreakOnMemoryToggled(bool checked);

private:
	breakpoint_handler* m_breakpoint_handler;

	std::weak_ptr<cpu_thread> cpu;
	std::shared_ptr<CPUDisAsm> m_disasm;

	QPointer<QAction> m_memory_breakpoint_toggle = nullptr;
};
