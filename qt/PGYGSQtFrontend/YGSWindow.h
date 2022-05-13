#ifndef YGSWINDOW_H
#define YGSWINDOW_H

#include <QUrl>
#include <QMainWindow>
#include <unordered_map>

QT_BEGIN_NAMESPACE
namespace Ui { class YGSWindow; }
QT_END_NAMESPACE

class QImage;
class QWebSocket;
class QNetworkAccessManager;

struct RenderOrder {
    enum class Code : int {
        NOP, CLEAR, DRAW,
    };

    static constexpr const char *CODE2STRING[] = {
        "N", "C", "D",
    };

    Code code{Code::NOP};
    std::vector<int> args;

    void fromJson(const QJsonValue &json);

    std::string stringify() const {
        std::string res(CODE2STRING[(int)code]);
        for (auto arg : args) {
            res.append(" ").append(std::to_string(arg));
        }
        return res;
    }
};

class YGSWindow : public QMainWindow {
    Q_OBJECT

public:
    YGSWindow(QWidget *parent = nullptr);
    ~YGSWindow();

    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
public slots:
    void wsOnMessage(const QString &msg);

private:
    QUrl ws_server_url_{"ws://127.0.0.1:8899"};
    QUrl http_server_url_{"http://127.0.0.1:8899"};
    Ui::YGSWindow *ui_;
    QWebSocket *ws_socket_{nullptr};
    QNetworkAccessManager *http_mgr{nullptr};
    std::unordered_map<int, QImage> img_cache_;
    std::vector<RenderOrder> rendering_orders_;
    double expend_factor_;
};
#endif // YGSWINDOW_H
