#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
	void on_vslide_p_sliderMoved(int position);

	void on_vslide_d_sliderMoved(int position);

	void on_vslide_i_sliderMoved(int position);

	void on_sb_p_crt_valueChanged(double arg1);

	void on_sb_d_crt_valueChanged(double arg1);

	void on_sb_i_crt_valueChanged(double arg1);

	void on_sb_p_max_valueChanged(double arg1);

	void on_sb_d_max_valueChanged(double arg1);

	void on_sb_i_max_valueChanged(double arg1);

	void on_sb_p_min_valueChanged(double arg1);

	void on_sb_d_min_valueChanged(double arg1);

	void on_sb_i_min_valueChanged(double arg1);

	void on_btn_refresh_clicked();

	void on_btn_connect_clicked();

	void on_horizontalSlider_valueChanged(int value);

	void on_sb_angle_off_valueChanged(double arg1);

	void on_btn_clear_cal_clicked();

	void on_btn_read_cal_clicked();

private:

	void init_serial_port(void);

	void param_chg_p(float value);

	void param_chg_d(float value);

	void param_chg_i(float value);

	void write_value(char *payload);

	void set_ok(bool ok);

	void serialize_int32(int src, char *dst);

    Ui::MainWindow *ui;
	QSerialPort port;
};

#endif // MAINWINDOW_H
