#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QClipboard>
#include <QDebug>

#define SVC_VENUS_SYSTEM  100
#define SVC_CHARGER_SOLAR 223
#define SVC_CHARGER_AC2DC 226
#define SVC_BATTERY_BMS   225
#define SVC_DIGITAL_IN1   1
#define SVC_DIGITAL_IN2   2
#define SVC_DIGITAL_IN3   3
#define SVC_DIGITAL_IN4   4

#define KEY(a, b)   (((a << 16) & 0xffff0000) | b)
#define UNIT(x)     ((x >> 16) & 0x0000ffff)
#define REGID(x)    (x & 0x0000ffff)
#define SCALE(x, s) (s != 0.0f ? x / s : x)
#define BIT(x)      (1 << (x))
#define HIBYTE(x)   ((quint8) ((x >> 8) & 0x00ff))
#define LOBYTE(x)   ((quint8) (x & 0x00ff))

/* ------------------------------------------------------------------------ */

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_modbus(this)
    , m_model(this)
    , m_index(0)
{
    ui->setupUi(this);
    ui->tableView->setModel(&m_model);
    ui->gbxUpdRegister->setEnabled(false);
    ui->btnClose->setEnabled(false);
    ui->btnOpen->setEnabled(true);

    ui->edIpAddress->setText("192.168.181.19");
    ui->edIpPort->setValue(502);

    connect(&m_timer, &QTimer::timeout, this, &MainWindow::onSendRequest);
    connect(&m_modbus, &QModbusTcpClient::errorOccurred, this, &MainWindow::onModbusError);
    connect(&m_modbus, &QModbusTcpClient::stateChanged, this, &MainWindow::onModbusState);

    m_packets.append({SVC_DIGITAL_IN1, 3420, 1, 1, tr("Digital In 1 Count")});
    m_packets.append({SVC_DIGITAL_IN1, 3422, 1, 1, tr("Digital In 1 State")});
    m_packets.append({SVC_DIGITAL_IN1, 3423, 1, 1, tr("Digital In 1 Alarm")});
    m_packets.append({SVC_DIGITAL_IN1, 3424, 1, 1, tr("Digital In 1 Type")});
    m_packets.append({SVC_DIGITAL_IN2, 3420, 1, 1, tr("Digital In 2 Count")});
    m_packets.append({SVC_DIGITAL_IN2, 3422, 1, 1, tr("Digital In 2 State")});
    m_packets.append({SVC_DIGITAL_IN2, 3423, 1, 1, tr("Digital In 2 Alarm")});
    m_packets.append({SVC_DIGITAL_IN2, 3424, 1, 1, tr("Digital In 2 Type")});
    m_packets.append({SVC_DIGITAL_IN3, 3420, 1, 1, tr("Digital In 3 Count")});
    m_packets.append({SVC_DIGITAL_IN3, 3422, 1, 1, tr("Digital In 3 State")});
    m_packets.append({SVC_DIGITAL_IN3, 3423, 1, 1, tr("Digital In 3 Alarm")});
    m_packets.append({SVC_DIGITAL_IN3, 3424, 1, 1, tr("Digital In 3 Type")});
    m_packets.append({SVC_DIGITAL_IN4, 3420, 1, 1, tr("Digital In 4 Count")});
    m_packets.append({SVC_DIGITAL_IN4, 3422, 1, 1, tr("Digital In 4 State")});
    m_packets.append({SVC_DIGITAL_IN4, 3423, 1, 1, tr("Digital In 4 Alarm")});
    m_packets.append({SVC_DIGITAL_IN4, 3424, 1, 1, tr("Digital In 4 Type")});

    m_packets.append({SVC_VENUS_SYSTEM, 806, 1, 1, tr("Relay-0 State")});
    m_packets.append({SVC_VENUS_SYSTEM, 807, 1, 1, tr("Relay-1 State")});
    m_packets.append({SVC_VENUS_SYSTEM, 840, 1, 10, tr("Battery Voltage (V)")});
    m_packets.append({SVC_VENUS_SYSTEM, 841, 1, 10, tr("Battery Current (A)")});
    m_packets.append({SVC_VENUS_SYSTEM, 842, 1, 1, tr("Battery Power (W)")});
    m_packets.append({SVC_VENUS_SYSTEM, 843, 1, 1, tr("Battery Capacity (%)")});
    m_packets.append({SVC_VENUS_SYSTEM, 844, 1, 1, tr("Battery State")});
    m_packets.append({SVC_VENUS_SYSTEM, 845, 1, -10, tr("Consumed (A/h)")});
    m_packets.append({SVC_VENUS_SYSTEM, 850, 1, 1, tr("PV Power (W)")});
    m_packets.append({SVC_VENUS_SYSTEM, 851, 1, 10, tr("PV Current (A)")});
    m_packets.append({SVC_VENUS_SYSTEM, 855, 1, 1, tr("Charger Power (W)")});
    m_packets.append({SVC_VENUS_SYSTEM, 860, 1, 1, tr("DC Power (W)")});

    m_packets.append({SVC_BATTERY_BMS, 258, 1, 1, tr("DC Power (W)")});
    m_packets.append({SVC_BATTERY_BMS, 259, 1, 100, tr("DC Voltage 1 (V)")});
    m_packets.append({SVC_BATTERY_BMS, 260, 1, 100, tr("DC Voltage 2 (V)")});
    m_packets.append({SVC_BATTERY_BMS, 261, 1, 10, tr("DC Current 1 (A)")});
    m_packets.append({SVC_BATTERY_BMS, 262, 1, 10, tr("Temperature (°C)")});
    m_packets.append({SVC_BATTERY_BMS, 263, 1, 100, tr("Mid. Voltage (V)")});
    m_packets.append({SVC_BATTERY_BMS, 264, 1, 100, tr("Mid. Deviation")});
    m_packets.append({SVC_BATTERY_BMS, 265, 1, -10, tr("Consumed (A/h)")});
    m_packets.append({SVC_BATTERY_BMS, 266, 1, 10, tr("Capacity (%)")});
    m_packets.append({SVC_BATTERY_BMS, 267, 1, 1, tr("Alarm")});
    m_packets.append({SVC_BATTERY_BMS, 268, 1, 1, tr("Alarm Low Voltage (V)")});
    m_packets.append({SVC_BATTERY_BMS, 269, 1, 1, tr("Alarm High Voltage (V)")});
    m_packets.append({SVC_BATTERY_BMS, 309, 1, 10, tr("Max. Capacity (Ah)")});
    m_packets.append({SVC_BATTERY_BMS, 1282, 1, 1, tr("State")});
    m_packets.append({SVC_BATTERY_BMS, 1283, 1, 1, tr("Error Code")});
    m_packets.append({SVC_BATTERY_BMS, 1284, 1, 1, tr("System Switch")});
    m_packets.append({SVC_BATTERY_BMS, 1285, 1, 1, tr("Balancing")});
    m_packets.append({SVC_BATTERY_BMS, 1286, 1, 1, tr("Nr. Of Batteries")});
    m_packets.append({SVC_BATTERY_BMS, 1287, 1, 1, tr("Batteries Parallel")});
    m_packets.append({SVC_BATTERY_BMS, 1288, 1, 1, tr("Batteries Series")});
    m_packets.append({SVC_BATTERY_BMS, 1289, 1, 1, tr("Nr. Of Cells/Battery")});
    m_packets.append({SVC_BATTERY_BMS, 1290, 1, 100, tr("Min. Cell Voltage (V)")});
    m_packets.append({SVC_BATTERY_BMS, 1291, 1, 100, tr("Max. Cell Voltage (V)")});
    m_packets.append({SVC_BATTERY_BMS, 1297, 1, 1, tr("Allow To Charge")});
    m_packets.append({SVC_BATTERY_BMS, 1298, 1, 1, tr("Allow To Discharge")});
    m_packets.append({SVC_BATTERY_BMS, 1319, 1, 1, tr("Mode")});
    m_packets.append({SVC_BATTERY_BMS, 1320, 1, 1, tr("Status")});

    m_packets.append({SVC_CHARGER_AC2DC, 2307, 1, 100, tr("Charge Voltage (V)")});
    m_packets.append({SVC_CHARGER_AC2DC, 2308, 1, 10, tr("Charge Current (A)")});
    m_packets.append({SVC_CHARGER_AC2DC, 2309, 1, 10, tr("Temperature (°C)")});
    m_packets.append({SVC_CHARGER_AC2DC, 2310, 1, 100, tr("Voltage 1 (V)")});
    m_packets.append({SVC_CHARGER_AC2DC, 2311, 1, 10, tr("Current 1 (A)")});
    m_packets.append({SVC_CHARGER_AC2DC, 2312, 1, 100, tr("Voltage 2 (V)")});
    m_packets.append({SVC_CHARGER_AC2DC, 2313, 1, 10, tr("Current 2 (A)")});
    m_packets.append({SVC_CHARGER_AC2DC, 2317, 1, 1, tr("Mode")});
    m_packets.append({SVC_CHARGER_AC2DC, 2318, 1, 1, tr("State")});
    m_packets.append({SVC_CHARGER_AC2DC, 2319, 1, 1, tr("Error Code")});
    m_packets.append({SVC_CHARGER_AC2DC, 2320, 1, 1, tr("Relay State")});
    m_packets.append({SVC_CHARGER_AC2DC, 2321, 1, 1, tr("Low Voltage (V)")});
    m_packets.append({SVC_CHARGER_AC2DC, 2322, 1, 1, tr("High Voltage (V)")});

    m_packets.append({SVC_CHARGER_SOLAR, 771, 1, 100, tr("Charge Voltage (V)")});
    m_packets.append({SVC_CHARGER_SOLAR, 772, 1, 10, tr("Charge Current (A)")});
    m_packets.append({SVC_CHARGER_SOLAR, 773, 1, 1, tr("Temperature (°C)")});
    m_packets.append({SVC_CHARGER_SOLAR, 774, 1, 1, tr("Mode")});
    m_packets.append({SVC_CHARGER_SOLAR, 775, 1, 1, tr("State")});
    m_packets.append({SVC_CHARGER_SOLAR, 776, 1, 100, tr("PV Voltage (V)")});
    m_packets.append({SVC_CHARGER_SOLAR, 777, 1, 10, tr("PV Current (A)")});
    m_packets.append({SVC_CHARGER_SOLAR, 788, 1, 10, tr("Error Code")});
    m_packets.append({SVC_CHARGER_SOLAR, 780, 1, 10, tr("Relay State")});
    m_packets.append({SVC_CHARGER_SOLAR, 789, 1, 10, tr("PV Power (W)")});
    m_packets.append({SVC_CHARGER_SOLAR, 790, 1, 10, tr("Power Consumed (KWh)")});
    m_packets.append({SVC_CHARGER_SOLAR, 3728, 1, 1, tr("Power Consumed (KWh)")});
    m_packets.append({SVC_CHARGER_SOLAR, 3730, 1, 1, tr("Power (W)")});
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onSendRequest()
{
    if (m_index >= m_packets.count()) {
        m_index = 0;
    }

    const auto mdut = QModbusDataUnit::HoldingRegisters;
    const CSCerboGxModel::TPacket p = m_packets[m_index];

    qDebug("READ> unit=%03d register=%04d %s", p.unitId, p.regId, p.name.toUtf8().constData());

    QModbusDataUnit mdu(mdut);
    mdu.setStartAddress(p.regId);
    mdu.setValueCount(p.size);

    QModbusReply* reply;
    if (!(reply = m_modbus.sendReadRequest(mdu, p.unitId))) {
        qCritical() << "Unable to send Modbus request";
        return;
    }

    reply->setProperty("packet", QVariant::fromValue(p));

    connect(reply, &QModbusReply::finished, this, &MainWindow::onModbusReply);
    connect(reply, &QModbusReply::destroyed, this, &MainWindow::onReplyDestroyed);
    connect(reply, &QModbusReply::errorOccurred, this, &MainWindow::onModbusError);

    m_index++;
}

void MainWindow::onModbusError(QModbusDevice::Error code)
{
    if (code == QModbusDevice::NoError) {
        return;
    }

    QString msg;
    switch (code) {
        case QModbusDevice::ReadError: {
            msg = tr("Read error");
            break;
        }
        case QModbusDevice::WriteError: {
            msg = tr("Write error");
            break;
        }
        case QModbusDevice::ConnectionError: {
            msg = tr("Connection error");
            break;
        }
        case QModbusDevice::ConfigurationError: {
            msg = tr("Configuration error");
            break;
        }
        case QModbusDevice::TimeoutError: {
            msg = tr("Timeout error");
            break;
        }
        case QModbusDevice::ProtocolError: {
            msg = tr("Protocol error");
            break;
        }
        case QModbusDevice::ReplyAbortedError: {
            msg = tr("Reply aborted error");
            break;
        }
        default: {
            msg = tr("Unknown error %1").arg(code);
            break;
        }
    }

    qWarning() << "MODBUS: Error:" << code << msg.toUtf8().constData();
}

void MainWindow::onModbusState(QModbusDevice::State state)
{
    qInfo() << "MODBUS: State changed:" << state;

    switch (state) {
        case QModbusDevice::ConnectingState: {
            ui->btnClose->setEnabled(true);
            ui->btnOpen->setEnabled(false);
            break;
        }
        case QModbusDevice::ConnectedState: {
            ui->gbxUpdRegister->setEnabled(true);
            if (!m_timer.isActive()) {
                m_timer.setInterval(1000);
                m_timer.setSingleShot(false);
                m_timer.setTimerType(Qt::CoarseTimer);
                m_timer.start();
            }
            break;
        }
        case QModbusDevice::ClosingState: {
            if (m_timer.isActive()) {
                m_timer.stop();
            }
            break;
        }
        case QModbusDevice::UnconnectedState: {
            ui->gbxUpdRegister->setEnabled(false);
            ui->btnClose->setEnabled(false);
            ui->btnOpen->setEnabled(true);
            break;
        }
    }
}

void MainWindow::onModbusReply()
{
    QModbusReply* reply;
    if (!(reply = dynamic_cast<QModbusReply*>(sender()))) {
        qCritical() << "MODBUS: Invalid reply object.";
        return;
    }

    if (reply->error() != QModbusDevice::NoError) {
        qCritical() << "MODBUS: Reply error:" << reply->error();
        reply->deleteLater();
        return;
    }

    const QVariant vp = reply->property("packet");
    if (vp.isNull() || !vp.isValid()) {
        qCritical() << "MODBUS: Invalid reply packet object.";
        reply->deleteLater();
        return;
    }

    const QModbusDataUnit mdu = reply->result();
    const CSCerboGxModel::TPacket p = vp.value<CSCerboGxModel::TPacket>();

    if (mdu.isValid() && mdu.valueCount() >= p.size) {
        m_model.updateRegister(p, mdu);
    }

    /* remove reply object */
    reply->deleteLater();
    return;
}

void MainWindow::onReplyDestroyed()
{
}

void MainWindow::on_btnOpen_clicked()
{
    if (m_modbus.state() != QModbusTcpClient::UnconnectedState) {
        return;
    }

    m_modbus.setConnectionParameter( //
       QModbusTcpClient::NetworkAddressParameter,
       ui->edIpAddress->text());
    m_modbus.setConnectionParameter( //
       QModbusTcpClient::NetworkPortParameter,
       ui->edIpPort->value());

    if (!m_modbus.connectDevice()) {
        qCritical() << "MODBUS: Unbale to connect device at " //
                    << ui->edIpAddress->text()                //
                    << "port:" << ui->edIpPort->value();
        return;
    }
}

void MainWindow::on_btnClose_clicked()
{
    if (m_modbus.state() == QModbusTcpClient::ConnectedState) {
        m_modbus.disconnectDevice();
    }
}

void MainWindow::on_btnSetRegValue_clicked()
{
    const auto mdut =
       (ui->rbHoldingReg->isChecked() //
           ? QModbusDataUnit::HoldingRegisters
           : QModbusDataUnit::InputRegisters);

    auto sendAsDataUnit = [this, mdut](const CSCerboGxModel::TPacket& p, quint16 v) {
        QModbusDataUnit mdu(mdut);
        mdu.setStartAddress(p.regId);
        mdu.setValues(QVector<quint16>() << v);

        qDebug("SEND> unit=%03d register=%04d value=%d %s", p.unitId, p.regId, v, p.name.toUtf8().constData());

        QModbusReply* reply;
        if (!(reply = m_modbus.sendWriteRequest(mdu, p.unitId))) {
            qCritical() << "Unable to send Modbus request";
            return;
        }

        reply->setProperty("packet", QVariant::fromValue(p));

        connect(reply, &QModbusReply::finished, this, &MainWindow::onModbusReply);
        connect(reply, &QModbusReply::destroyed, this, &MainWindow::onReplyDestroyed);
        connect(reply, &QModbusReply::errorOccurred, this, &MainWindow::onModbusError);
    };

    auto sendAsRawRequest = [this](const CSCerboGxModel::TPacket& p, quint16 v) {
        quint16 size = 2;
        QByteArray data;

        /* 1st: Register high btye */
        data.append(HIBYTE(p.regId));
        /* 2nd: Register low byte */
        data.append(LOBYTE(p.regId));
        /* 1st: Value high btye */
        data.append(HIBYTE(size));
        /* 2nd: Value low byte */
        data.append(LOBYTE(size));
        /* 1st: Value high btye */
        data.append(HIBYTE(v));
        /* 2nd: Value low byte */
        data.append(LOBYTE(v));

        qDebug("SEND> unit=%03d register=%04d value=%d %s", p.unitId, p.regId, v, p.name.toUtf8().constData());

        QModbusReply* reply;

        QModbusRequest req(QModbusRequest::WriteSingleRegister, data);
        if (!(reply = m_modbus.sendRawRequest(req, p.unitId))) {
            qCritical() << "Unable to send Modbus request";
            return;
        }

        reply->setProperty("packet", QVariant::fromValue(p));

        connect(reply, &QModbusReply::finished, this, &MainWindow::onModbusReply);
        connect(reply, &QModbusReply::destroyed, this, &MainWindow::onReplyDestroyed);
        connect(reply, &QModbusReply::errorOccurred, this, &MainWindow::onModbusError);
    };

    foreach (auto p, m_packets) {
        if (p.unitId == (quint8) ui->edUnitId->value() && p.regId == ui->edRegister->value()) {
            if (ui->cbxSendAsMbReq->isChecked()) {
                sendAsRawRequest(p, ui->edRegValue->value());
            }
            else {
                sendAsDataUnit(p, ui->edRegValue->value());
            }
            return;
        }
    }
}

void MainWindow::on_tableView_clicked(const QModelIndex& index)
{
    QVariant v = m_model.data(index, Qt::UserRole);
    if (v.isNull() || !v.isValid()) {
        return;
    }
    CSCerboGxModel::TPacket p = v.value<CSCerboGxModel::TPacket>();
    ui->edUnitId->setValue(p.unitId);
    ui->edRegister->setValue(p.regId);
    ui->edRegValue->setValue(0);
}

void MainWindow::on_tableView_doubleClicked(const QModelIndex& index)
{
    QVariant v = m_model.data(index, Qt::UserRole);
    if (v.isNull() || !v.isValid()) {
        return;
    }
    CSCerboGxModel::TPacket p = v.value<CSCerboGxModel::TPacket>();
    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setText(QString::number(p.regId));
}

void MainWindow::on_rbHoldingReg_clicked(bool checked)
{
    ui->rbInputReg->setChecked(!checked);
}

void MainWindow::on_rbInputReg_clicked(bool checked)
{
    ui->rbHoldingReg->setChecked(!checked);
}

/* ------------------------------------------------------------------------ */
/* ------------------------------------------------------------------------ */

CSCerboGxModel::CSCerboGxModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

int CSCerboGxModel::columnCount(const QModelIndex&) const
{
    return 4;
}

int CSCerboGxModel::rowCount(const QModelIndex&) const
{
    return m_data.count();
}

/* first unit Id, second register Id */
QPair<quint16, quint16> CSCerboGxModel::toRegId(const QModelIndex& index) const
{
    QList<uint> registers = m_data.keys();
    if (index.row() >= registers.count()) {
        return {};
    }
    const uint fncode = registers[index.row()];
    const quint16 unitId = ((fncode >> 16) & 0xffff);
    const quint16 regId = (fncode & 0x0000ffff);
    return {unitId, regId};
}

QVariant CSCerboGxModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Orientation::Horizontal) {
        if (role == Qt::DisplayRole) {
            switch (section) {
                case 0: {
                    return tr("Unit");
                }
                case 1: {
                    return tr("Register");
                }
                case 2: {
                    return tr("Value");
                }
                case 3: {
                    return tr("Remarks");
                }
            }
        }
        else if (role == Qt::TextAlignmentRole) {
            return Qt::AlignLeft;
        }
    }
    return QVariant();
}

QVariant CSCerboGxModel::data(const QModelIndex& index, int role) const
{
    QList<uint> registers = m_data.keys();
    if (index.row() >= registers.count()) {
        return QVariant();
    }

    const uint key = registers[index.row()];
    const QModbusDataUnit mdu = m_data[key].second;
    const TPacket p = m_data[key].first;

    switch (role) {
        case Qt::UserRole: {
            return QVariant::fromValue(p);
            break;
        }
        case Qt::DisplayRole: {
            switch (index.column()) {
                case 0: {
                    switch (p.unitId) {
                        case SVC_VENUS_SYSTEM:
                            return tr("SYS");
                        case SVC_CHARGER_SOLAR:
                            return tr("SOL");
                        case SVC_CHARGER_AC2DC:
                            return tr("CHR");
                        case SVC_BATTERY_BMS:
                            return tr("BMS");
                        case SVC_DIGITAL_IN1:
                            return tr("DI1");
                        case SVC_DIGITAL_IN2:
                            return tr("DI2");
                        case SVC_DIGITAL_IN3:
                            return tr("DI3");
                        case SVC_DIGITAL_IN4:
                            return tr("DI4");
                    }
                    return tr("%1").arg(p.unitId, 3, 10, QChar('0'));
                }
                case 1: {
                    return tr("%1: %2").arg(p.regId, 5, 10, QChar('0')).arg(p.name);
                }
                case 2: {
                    /* empty variant object */
                    if (!mdu.isValid() || mdu.valueCount() < p.size)
                        return QVariant();

                    /* Digital Input */
                    if (p.unitId >= SVC_DIGITAL_IN1 && p.unitId <= SVC_DIGITAL_IN4) {
                        /* State register */
                        if (p.regId == 3422) {
                            return digitalInputState(mdu.value(0));
                        }
                        /* Alarm register */
                        if (p.regId == 3423) {
                            return digitalInputAlarm(mdu.value(0));
                        }
                        if (p.regId == 3424) {
                            return digitalInputType(mdu.value(0));
                        }
                    }

                    switch (p.unitId) {
                        /* System */
                        case SVC_VENUS_SYSTEM: {
                            switch (p.regId) {
                                /* Relay State */
                                case 806:
                                case 807: {
                                    return relayState(mdu.value(0));
                                }
                                /* Battery State */
                                case 844: {
                                    return batteryState(mdu.value(0));
                                }
                            }
                            break;
                        }
                        /* Ac/Dc Charger */
                        case SVC_CHARGER_AC2DC: {
                            switch (p.regId) {
                                /* device mode */
                                case 2317: {
                                    return acdcChargerMode(mdu.value(0));
                                }
                                /* device state */
                                case 2318: {
                                    return acdcChargerState(mdu.value(0));
                                }
                            }
                            break;
                        }
                        /* Solar Charger */
                        case SVC_CHARGER_SOLAR: {
                            switch (p.regId) {
                                /* op mode */
                                case 774: {
                                    return solarOpMode(mdu.value(0));
                                }
                                /* work state */
                                case 775: {
                                    return solarWorkState(mdu.value(0));
                                }
                            }
                            break;
                        }
                    }

                    return columnValue(p.scale, mdu.value(0));
                }
                case 3: {
                    break;
                }
            }
            break;
        }
    }
    return QVariant();
}

void CSCerboGxModel::beginUpdate()
{
    beginResetModel();
}

void CSCerboGxModel::endUpdate()
{
    endResetModel();
}

void CSCerboGxModel::updateRegister(const TPacket& p, const QModbusDataUnit& mdu)
{
    beginResetModel();
    m_data[KEY(p.unitId, p.regId)] = {p, mdu};
    endResetModel();
}

inline QVariant CSCerboGxModel::columnValue(float scale, quint16 value) const
{
    qreal f = SCALE((int) value, scale);
    return QVariant::fromValue(QString::number(f));
}

inline QVariant CSCerboGxModel::digitalInputType(uint type) const
{
    switch (type) {
        case 2: {
            return QVariant::fromValue(tr("Dor"));
        }
        case 3: {
            return QVariant::fromValue(tr("Bilge pump"));
        }
        case 4: {
            return QVariant::fromValue(tr("Bilge alarm"));
        }
        case 5: {
            return QVariant::fromValue(tr("Burglar alarm"));
        }
        case 6: {
            return QVariant::fromValue(tr("Smoke alarm"));
        }
        case 7: {
            return QVariant::fromValue(tr("Fire alarm"));
        }
        case 8: {
            return QVariant::fromValue(tr("CO2 alarm"));
        }
        case 9: {
            return QVariant::fromValue(tr("Signal"));
        }
    }
    return {type};
}

inline QVariant CSCerboGxModel::batteryState(uint state) const
{
    switch (state) {
        case 0: {
            return QVariant::fromValue(tr("Idle"));
        }
        case 1: {
            return QVariant::fromValue(tr("Charging"));
        }
        case 2: {
            return QVariant::fromValue(tr("Discharging"));
        }
    }
    return {state};
}

inline QVariant CSCerboGxModel::relayState(uint state) const
{
    switch (state) {
        case 0: {
            return QVariant::fromValue(tr("Off"));
        }
        case 1: {
            return QVariant::fromValue(tr("On"));
        }
    }
    return {state};
}

inline QVariant CSCerboGxModel::digitalInputAlarm(uint state) const
{
    switch (state) {
        case 0:
            return QVariant::fromValue(tr("No alarm"));
        case 2:
            return QVariant::fromValue(tr("Alarm"));
    }
    return {state};
}

inline QVariant CSCerboGxModel::digitalInputState(uint state) const
{
    switch (state) {
        case 0:
            return QVariant::fromValue(tr("Low"));
        case 1:
            return QVariant::fromValue(tr("High"));
        case 2:
            return QVariant::fromValue(tr("Off"));
        case 3:
            return QVariant::fromValue(tr("On"));
        case 4:
            return QVariant::fromValue(tr("No"));
        case 5:
            return QVariant::fromValue(tr("Yes"));
        case 6:
            return QVariant::fromValue(tr("Open"));
        case 7:
            return QVariant::fromValue(tr("Closed"));
        case 8:
            return QVariant::fromValue(tr("Alarm"));
        case 9:
            return QVariant::fromValue(tr("OK"));
        case 10:
            return QVariant::fromValue(tr("Listening"));
        case 11:
            return QVariant::fromValue(tr("Stopped"));
    }
    return {state};
}

inline QVariant CSCerboGxModel::acdcChargerMode(uint state) const
{
    return QVariant::fromValue(tr(state ? "Power Supply" : "AC/DC Charger"));
}

inline QVariant CSCerboGxModel::acdcChargerState(uint state) const
{
    switch (state) {
        case 0:
            return QVariant::fromValue(tr("Off"));
        case 1:
            return QVariant::fromValue(tr("Low Power Mode"));
        case 2:
            return QVariant::fromValue(tr("Fault"));
        case 3:
            return QVariant::fromValue(tr("Bulk"));
        case 4:
            return QVariant::fromValue(tr("Absorption"));
        case 5:
            return QVariant::fromValue(tr("Float"));
        case 6:
            return QVariant::fromValue(tr("Storage"));
        case 7:
            return QVariant::fromValue(tr("Equalize"));
        case 8:
            return QVariant::fromValue(tr("Passthru"));
        case 9:
            return QVariant::fromValue(tr("Inverting"));
        case 10:
            return QVariant::fromValue(tr("Power assist"));
        case 11:
            return QVariant::fromValue(tr("Power supply mode"));
        case 252:
            return QVariant::fromValue(tr("External control"));
    }
    return {state};
}

inline QVariant CSCerboGxModel::solarWorkState(uint state) const
{
    switch (state) {
        case 0:
            return QVariant::fromValue(tr("Off"));
        case 2:
            return QVariant::fromValue(tr("Fault"));
        case 3:
            return QVariant::fromValue(tr("Bulk"));
        case 4:
            return QVariant::fromValue(tr("Absorption"));
        case 5:
            return QVariant::fromValue(tr("Float"));
        case 6:
            return QVariant::fromValue(tr("Storage"));
        case 7:
            return QVariant::fromValue(tr("Equalize"));
        case 11:
            return QVariant::fromValue(tr("Other (Hub-1)"));
        case 252:
            return QVariant::fromValue(tr("External control"));
    }
    return {state};
}

inline QVariant CSCerboGxModel::solarOpMode(uint mode) const
{
    switch (mode) {
        case 0:
            return QVariant::fromValue(tr("Off"));
        case 1:
            return QVariant::fromValue(tr("Voltage/Current limited"));
        case 2:
            return QVariant::fromValue(tr("MPPT"));
        case 3:
            return QVariant::fromValue(tr("On"));
        case 255:
            return QVariant::fromValue(tr("Not available"));
    }
    return {mode};
}
