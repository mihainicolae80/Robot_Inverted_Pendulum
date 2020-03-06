#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPortInfo>
#include <QDebug>
#include <stdint.h>
#include <iostream>



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

	// populate port list
	on_btn_refresh_clicked();

	init_serial_port();
	set_ok(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::init_serial_port(void)
{
	port.setBaudRate(QSerialPort::Baud38400);
	port.setDataBits(QSerialPort::Data8);
	port.setParity(QSerialPort::NoParity);
	port.setStopBits(QSerialPort::OneStop);
	port.setFlowControl(QSerialPort::NoFlowControl);
}

void MainWindow::param_chg_p(float value)
{
	char payload[7];
	int32_t intvalue;

	payload[0] = 'p';
	payload[1] = '=';
	intvalue = static_cast<int>(value * 1000.0f);
	memcpy(payload + 2, &intvalue, 4);
	payload[6] = '\n';
	write_value(payload);
}

void MainWindow::param_chg_d(float value)
{
	char payload[7];
	int32_t intvalue;

	payload[0] = 'd';
	payload[1] = '=';
	intvalue = static_cast<int>(value * 1000.0f);
	memcpy(payload + 2, &intvalue, 4);
	payload[6] = '\n';
	write_value(payload);
}

void MainWindow::param_chg_i(float value)
{
	char payload[7];
	int32_t intvalue;

	payload[0] = 'i';
	payload[1] = '=';
	intvalue = static_cast<int>(value * 1000.0f) ;
	memcpy(payload + 2, &intvalue, 4);
	payload[6] = '\n';
	write_value(payload);
}

void MainWindow::write_value(char *payload)
{
	char a, b;

	set_ok(false);

	if (port.isOpen()) {
		port.write(payload, 7);
		if (!port.waitForBytesWritten(100)) {
			set_ok(false);
			return;
		}

		a = ' ';
		b = ' ';
		std::cout << "RX:";
		do {
			if (!port.waitForReadyRead(50)) {
				set_ok(false);
				port.clear();
				return;
			}
			b = a;
			port.read(&a, 1);
			std::cout << a;
		} while (!((a == 'k') && (b == 'o')));

		std::cout << std::endl;

		set_ok(true);
	} else {
		qDebug() << "Port not open!";
	}
}

void MainWindow::set_ok(bool ok)
{
	if (ok) {
		// color label GREEN
		ui->lb_ok->setStyleSheet("color: rgb(0, 222, 0);");
	} else {
		// color label GRAY
		ui->lb_ok->setStyleSheet("color: rgb(100, 100, 100);");
	}
}

void MainWindow::serialize_int32(int src, char *dst)
{
	dst[0] = static_cast<char>((src & 0xff));
	dst[1] = static_cast<char>(((src >> 8) & 0xff));
	dst[2] = static_cast<char>(((src >> 16) & 0xff));
	dst[3] = static_cast<char>(((src >> 24) & 0xff));
}

void MainWindow::on_vslide_p_sliderMoved(int position)
{
	ui->sb_p_crt->blockSignals(true);
	ui->sb_p_crt->setValue(static_cast<double>(position) / 1000.0);
	ui->sb_p_crt->blockSignals(false);

	param_chg_p(static_cast<float>(position) / 1000.0f);
}

void MainWindow::on_vslide_d_sliderMoved(int position)
{
	ui->sb_d_crt->blockSignals(true);
	ui->sb_d_crt->setValue(static_cast<double>(position) / 1000.0);
	ui->sb_d_crt->blockSignals(false);

	param_chg_d(static_cast<float>(position) / 1000.0f);
}

void MainWindow::on_vslide_i_sliderMoved(int position)
{
	ui->sb_i_crt->blockSignals(true);
	ui->sb_i_crt->setValue(static_cast<double>(position) / 1000.0);
	ui->sb_i_crt->blockSignals(false);

	param_chg_i(static_cast<float>(position) / 1000.0f);
}

void MainWindow::on_sb_p_crt_valueChanged(double arg1)
{
	ui->vslide_p->blockSignals(true);
	ui->vslide_p->setValue(static_cast<int>(arg1 * 1000));
	ui->vslide_p->blockSignals(false);

	param_chg_p(static_cast<float>(arg1));
}

void MainWindow::on_sb_d_crt_valueChanged(double arg1)
{
	ui->vslide_d->blockSignals(true);
	ui->vslide_d->setValue(static_cast<int>(arg1 * 1000));
	ui->vslide_d->blockSignals(false);

	param_chg_d(static_cast<float>(arg1));
}

void MainWindow::on_sb_i_crt_valueChanged(double arg1)
{
	ui->vslide_i->blockSignals(true);
	ui->vslide_i->setValue(static_cast<int>(arg1 * 1000));
	ui->vslide_i->blockSignals(false);

	param_chg_i(static_cast<float>(arg1));
}

void MainWindow::on_sb_p_max_valueChanged(double arg1)
{
	int intvalue = static_cast<int>(arg1 * 1000);

	// if max value lower that min value
	if (arg1 <= ui->sb_p_min->value()) {
		// ignore value
		return;
	}

	// update slider if needed
	if (ui->vslide_p->value() > intvalue) {
		// this will also set the associated spinbox
		ui->vslide_p->setValue(intvalue);
	}

	// update max values
	ui->vslide_p->setMaximum(intvalue);
	ui->sb_p_crt->setMaximum(arg1);
}

void MainWindow::on_sb_d_max_valueChanged(double arg1)
{
	int intvalue = static_cast<int>(arg1 * 1000);

	// if max value lower that min value
	if (arg1 <= ui->sb_d_min->value()) {
		// ignore value
		return;
	}

	// update slider if needed
	if (ui->vslide_d->value() > intvalue) {
		// this will also set the associated spinbox
		ui->vslide_d->setValue(intvalue);
	}

	// update max values
	ui->vslide_d->setMaximum(intvalue);
	ui->sb_d_crt->setMaximum(arg1);
}

void MainWindow::on_sb_i_max_valueChanged(double arg1)
{
	int intvalue = static_cast<int>(arg1 * 1000);

	// if max value lower that min value
	if (arg1 <= ui->sb_i_min->value()) {
		// ignore value
		return;
	}

	// update slider if needed
	if (ui->vslide_i->value() > intvalue) {
		// this will also set the associated spinbox
		ui->vslide_i->setValue(intvalue);
	}

	// update max values
	ui->vslide_i->setMaximum(intvalue);
	ui->sb_i_crt->setMaximum(arg1);
}

void MainWindow::on_sb_p_min_valueChanged(double arg1)
{
	int intvalue = static_cast<int>(arg1 * 1000);

	// if max value lower that min value
	if (arg1 >= ui->sb_p_max->value()) {
		// ignore value
		return;
	}

	// update slider if needed
	if (ui->vslide_p->value() < intvalue) {
		// this will also set the associated spinbox
		ui->vslide_p->setValue(intvalue);
	}

	// update max values
	ui->vslide_p->setMinimum(intvalue);
	ui->sb_p_crt->setMinimum(arg1);
}

void MainWindow::on_sb_d_min_valueChanged(double arg1)
{
	int intvalue = static_cast<int>(arg1 * 1000);

	// if max value lower that min value
	if (arg1 >= ui->sb_d_max->value()) {
		// ignore value
		return;
	}

	// update slider if needed
	if (ui->vslide_d->value() < intvalue) {
		// this will also set the associated spinbox
		ui->vslide_d->setValue(intvalue);
	}

	// update max values
	ui->vslide_d->setMinimum(intvalue);
	ui->sb_d_crt->setMinimum(arg1);
}

void MainWindow::on_sb_i_min_valueChanged(double arg1)
{
	int intvalue = static_cast<int>(arg1 * 1000);

	// if max value lower that min value
	if (arg1 >= ui->sb_i_max->value()) {
		// ignore value
		return;
	}

	// update slider if needed
	if (ui->vslide_i->value() < intvalue) {
		// this will also set the associated spinbox
		ui->vslide_i->setValue(intvalue);
	}

	// update max values
	ui->vslide_i->setMinimum(intvalue);
	ui->sb_i_crt->setMinimum(arg1);
}

void MainWindow::on_btn_refresh_clicked()
{
	QString selected = ui->cb_ports->currentText();
	bool still_present;


	// check if selected port still present
	still_present = false;
	if (QSerialPortInfo::availablePorts().size() > 0) {
		for (QSerialPortInfo info : QSerialPortInfo::availablePorts())  {
			if (0 == QString::compare(info.portName(), selected, Qt::CaseInsensitive)) {
				still_present = true;
			}
		}
	}
	// add ports
	ui->cb_ports->clear();
	ui->cb_ports->blockSignals(true);
	// add previously selected port
	if (still_present) {
		ui->cb_ports->addItem(selected);
	}
	// add the rest of the ports
	for (QSerialPortInfo info : QSerialPortInfo::availablePorts()) {
		if (0 != QString::compare(info.portName(), selected, Qt::CaseInsensitive)) {
			ui->cb_ports->addItem(info.portName());
		}
	}
	ui->cb_ports->blockSignals(false);
}

void MainWindow::on_btn_connect_clicked()
{
	QString portname;

	// attempt to open
	if (!port.isOpen()) {
		// attempt to connect to serial port
		portname = ui->cb_ports->currentText();

		if ((nullptr == portname) || (portname.size() < 1)) {
			set_ok(false);
			return;
		}

		port.setPortName(portname);
		if (port.open(QIODevice::ReadWrite)) {
			ui->btn_connect->setText("Discon.");
			ui->btn_connect->update();
			set_ok(true);
			// @success connect -> enable interface
			ui->frame_commands->setEnabled(true);
			ui->cb_ports->setEnabled(false);
			ui->btn_refresh->setEnabled(false);
		} else {
			qDebug() << "Failed to connect!";
			set_ok(false);
		}
	} else {
		port.close();
		ui->btn_connect->setText("Connect");
		set_ok(false);
		// @success disconnect -> disable interface
		ui->frame_commands->setEnabled(false);
		ui->cb_ports->setEnabled(true);
		ui->btn_refresh->setEnabled(true);
	}
}

void MainWindow::on_horizontalSlider_valueChanged(int value)
{
	int intvalue;
	char payload[7];

	intvalue = (value - 9000) * 10;
	payload[0] = 'a';
	payload[1] = '=';
	//memcpy(payload + 2, &intvalue, 4);
	//serialize_int32(intvalue, payload + 2);
	payload[2] = static_cast<char>(intvalue & 0xFF);
	payload[3] = static_cast<char>((intvalue >> 8) & 0xFF);
	payload[4] = static_cast<char>((intvalue >> 16) & 0xFF);
	payload[5] = static_cast<char>((intvalue >> 24) & 0xFF);
	payload[6] = '\n';
	write_value(payload);

	ui->sb_angle_off->blockSignals(true);
	ui->sb_angle_off->setValue(static_cast<double>(value - 9000) / 100);
	ui->sb_angle_off->blockSignals(false);
}

void MainWindow::on_sb_angle_off_valueChanged(double arg1)
{
	//ui->horizontalSlider->blockSignals(true);
	ui->horizontalSlider->setValue(static_cast<int>(arg1 * 100.0 + 9000));
	//ui->horizontalSlider->blockSignals(false);
}

void MainWindow::on_btn_clear_cal_clicked()
{
	ui->te_cal->clear();
}

void MainWindow::on_btn_read_cal_clicked()
{
	char payload[] = "c\n";
	char payload_rx[22];
	int i;

	port.write(payload, 2);
	if (!port.waitForBytesWritten(100)) {
		set_ok(false);
		return;
	}

	if (!port.waitForReadyRead(100)) {
		set_ok(false);
		port.clear();
		return;
	}

	// read data from device
	port.read(payload_rx, 22);

	// print data
	for (i = 0; i < 22; i++) {
		ui->te_cal->append(QString().setNum(static_cast<uint8_t>(payload_rx[i]), 10) + " ");
	}

	set_ok(true);
}

void MainWindow::on_btn_set_calib_clicked()
{
	char payload[] = "calib\n";
	char payload_rx[22];


	set_ok(false);
	port.write(payload, 6);
	if (!port.waitForBytesWritten(100)) {
		return;
	}

	if (!port.waitForReadyRead(100)) {
		port.clear();
		return;
	}

	// read data from device
	port.read(payload_rx, 22);

	set_ok(true);
}
