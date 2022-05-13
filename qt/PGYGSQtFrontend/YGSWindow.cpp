#include "YGSWindow.h"
#include "ui_YGSWindow.h"

#include <cmath>
#include <utility>
#include <QKeyEvent>
#include <QImage>
#include <QPainter>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>

YGSWindow::YGSWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui_(new Ui::YGSWindow) {
    qDebug() << __func__;
    ui_->setupUi(this);
    ws_socket_ = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
    http_mgr = new QNetworkAccessManager(this);

    ws_socket_->open(ws_server_url_);
    connect(ws_socket_, &QWebSocket::connected, [this]() {
        ws_socket_->sendTextMessage("C PGZXB R002 tank_game;");
        ws_socket_->sendTextMessage("S;");
        connect(ws_socket_, &QWebSocket::textMessageReceived, this, &YGSWindow::wsOnMessage);
    });
}

YGSWindow::~YGSWindow() {
    delete ui_;
}

void YGSWindow::paintEvent(QPaintEvent *) {
    QPainter p(this);

    for (const auto &order : rendering_orders_) {
        const auto &[op, args] = order;
        if (op == RenderOrder::Code::CLEAR) (void(0));
        else if (op == RenderOrder::Code::DRAW) {
            if (args.size() == 6) {
                int resID = args[0];
                auto iter = img_cache_.find(resID);
                if (iter != img_cache_.end()) {
                    QRect area;
                    area.setX(args[1]);
                    area.setY(args[2]);
                    area.setWidth(args[3]);
                    area.setHeight(args[4]);
                    int theta = args[5];
                    if (resID == 0) {
                        double widthFactor = ui_->gameAreaWidget->width() / (double)area.width();
                        double heightFactor = ui_->gameAreaWidget->height() / (double)area.height();
                        expend_factor_ = std::min(widthFactor, heightFactor);
                    }
                    QRect trueArea;
                    trueArea.setX(area.x() * expend_factor_);
                    trueArea.setY(area.y() * expend_factor_);
                    trueArea.setWidth(area.width() * expend_factor_);
                    trueArea.setHeight(area.height() * expend_factor_);

                    const double width = trueArea.width();
                    const double height = trueArea.height();
                    const double center_xpos = trueArea.x() + width / 2.f;
                    const double center_ypos = trueArea.y() + height / 2.f;
                    p.save();
                    p.translate(center_xpos, center_ypos);
                    p.rotate(theta / 1000.0 + 90.0);
                    p.translate(-center_xpos, -center_ypos);
                    p.drawImage(trueArea, iter->second, iter->second.rect());
                    p.restore();
                }
            }
        }
    }
    rendering_orders_.clear();
}

void YGSWindow::keyPressEvent(QKeyEvent * event) {
#define XXX(qtKey, ygsKey) \
    if (event->key() == qtKey) { \
        ws_socket_->sendTextMessage(QString("I %1;").arg(ygsKey)); \
    }
    XXX(Qt::Key_W, 1 << 8);
    XXX(Qt::Key_S, 1 << 10);
    XXX(Qt::Key_A, 1 << 12);
    XXX(Qt::Key_D, 1 << 14);
    XXX(Qt::Key_Up, 1 << 8);
    XXX(Qt::Key_Down, 1 << 10);
    XXX(Qt::Key_Left, 1 << 12);
    XXX(Qt::Key_Right, 1 << 14);
    XXX(Qt::Key_P, 1 << 16);
#undef XXX
}

void YGSWindow::wsOnMessage(const QString & msg) {
    auto jsonDoc = QJsonDocument::fromJson(msg.toUtf8());
    auto respData = jsonDoc.object();

    auto iter = respData.find("code");
    if (iter == respData.end()) {
        QMessageBox::warning(this, "", "请求错误");
        return;
    }

    if (iter.value().toInt() != 0) {
        auto iterMsg = respData.find("msg");
        QString content = "请求错误";
        if (iterMsg != respData.end()) {
            content = iterMsg.value().toString();
        }
        QMessageBox::warning(this, "", content);
        return;
    }

    if (respData.contains("data")) {
        auto data = respData["data"].toObject();
        if (data.contains("resources")) {
            Q_ASSERT(data["resources"].isArray());
            auto resources = data["resources"].toArray();
            for (const auto &res : resources) {
                Q_ASSERT(res.isObject());
                auto resObj = res.toObject();
                if (resObj.contains("id") && resObj.contains("path")) {
                    auto id = resObj["id"].toInt();
                    auto path = resObj["path"].toString();
                    QNetworkRequest resq(http_server_url_.toString() + "/" + path);
                    QNetworkReply *reply = http_mgr->get(resq);
                    connect(reply, &QNetworkReply::finished, [this, id, reply]() {
                       if (reply->error() != QNetworkReply::NoError) {
                           QMessageBox::warning(this, "", reply->errorString());
                       }
                       auto & img = img_cache_[id];
                       img.loadFromData(reply->readAll());
                       qDebug() << "Get resource: " << id;
                    });
                }
            }
        }

        if (data.contains("orders")) {
            auto arr = data["orders"].toArray();
            for (const auto &e : arr) {
                RenderOrder order;
                order.fromJson(e);
                rendering_orders_.emplace_back(std::move(order));
            }
            this->update();
        }
    }
}

void RenderOrder::fromJson(const QJsonValue & json) {
    // Unsafe
    Q_ASSERT(json.isObject());
    auto obj = json.toObject();
    Q_ASSERT(obj.contains("op"));
    Q_ASSERT(obj.contains("args"));

    auto op = obj["op"].toString();
    for (std::size_t i = 0; i < std::size(CODE2STRING); ++i) {
        if (op == CODE2STRING[i]) {
            this->code = (Code)i;
            break;
        }
    }

    auto args = obj["args"].toArray();
    for (const auto &arg : args) {
        this->args.push_back(arg.toInt());
    }
}
