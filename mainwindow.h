#pragma once
#include <QAbstractTableModel>
#include <QMainWindow>
#include <QModbusClient>
#include <QModbusDataUnit>
#include <QModbusDataUnitMap>
#include <QModbusReply>
#include <QModbusTcpClient>
#include <QSharedData>
#include <QTimer>

QT_BEGIN_NAMESPACE

namespace Ui {
class MainWindow;
}

QT_END_NAMESPACE

class CSCerboGxModel: public QAbstractTableModel
{
    Q_OBJECT

public:
    enum TValueType {
        ValueName,
        DataValue,
    };
    Q_ENUM(TValueType)

    struct TPacket {
        uint unitId;
        quint16 regId;
        quint16 size;
        qreal scale;
        QString name;
    };

    explicit CSCerboGxModel(QObject* parent = nullptr);
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QPair<quint16, quint16> toRegId(const QModelIndex& index) const;
    void updateRegister(const TPacket& p, const QModbusDataUnit& mdu);
    void beginUpdate();
    void endUpdate();

private:
    QMap<quint32, QPair<TPacket, QModbusDataUnit>> m_data;

private:
    inline QVariant columnValue(float scale, quint16 value) const;
    inline QVariant digitalInputType(uint type) const;
    inline QVariant digitalInputState(uint state) const;
    inline QVariant digitalInputAlarm(uint state) const;
    inline QVariant batteryState(uint state) const;
    inline QVariant relayState(uint state) const;
    inline QVariant acdcChargerMode(uint state) const;
    inline QVariant acdcChargerState(uint state) const;
    inline QVariant solarWorkState(uint state) const;
    inline QVariant solarOpMode(uint mode) const;
};
Q_DECLARE_METATYPE(CSCerboGxModel::TValueType)
Q_DECLARE_METATYPE(CSCerboGxModel::TPacket)

class MainWindow: public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onSendRequest();
    void onModbusError(QModbusDevice::Error);
    void onModbusState(QModbusDevice::State);
    void onModbusReply();
    void onReplyDestroyed();
    void on_btnOpen_clicked();
    void on_btnClose_clicked();
    void on_btnSetRegValue_clicked();
    void on_tableView_clicked(const QModelIndex& index);
    void on_tableView_doubleClicked(const QModelIndex& index);
    void on_rbHoldingReg_clicked(bool checked);
    void on_rbInputReg_clicked(bool checked);

private:
    Ui::MainWindow* ui;
    QModbusTcpClient m_modbus;
    CSCerboGxModel m_model;
    QList<CSCerboGxModel::TPacket> m_packets;
    QTimer m_timer;
    int m_index;
};
