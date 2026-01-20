#include "splashscreen.h"
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QPaintEvent>
#include <cmath>

SplashScreen::SplashScreen(const QPixmap &pixmap, Qt::WindowFlags f)
    : QSplashScreen(pixmap, f), messageAlignment(Qt::AlignLeft), messageColor(Qt::black)
{
    messageText = "";
}

void SplashScreen::showMessageWithShadow(const QString &msg, Qt::Alignment align, const QColor &color)
{
    messageText = msg;
    messageAlignment = align;
    messageColor = color;
    // Trigger repaint - this matches the original develop implementation
    // DO NOT call Qt's showMessage() which crashes during wallet loading
    update();
}

void SplashScreen::paintEvent(QPaintEvent *event)
{
    // Call parent to draw the pixmap background
    QSplashScreen::paintEvent(event);
    
    if (messageText.isEmpty())
        return;
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    
    // Set up font
    QFont font = painter.font();
    font.setBold(true);
    // Check if font size is valid before modifying (pointSize() can return -1 for pixel-based fonts)
    int currentSize = font.pointSize();
    if (currentSize > 0) {
        font.setPointSize(currentSize + 1); // Slightly larger for visibility
    } else {
        // If pixel-based font, use pixelSize instead
        int pixelSize = font.pixelSize();
        if (pixelSize > 0) {
            font.setPixelSize(pixelSize + 1);
        } else {
            // Default to reasonable size if both are invalid
            font.setPointSize(10);
        }
    }
    painter.setFont(font);
    
    QFontMetrics fm(font);
    QRect widgetRect = rect();
    
    // Calculate text position based on alignment - match QSplashScreen behavior
    int x = 0, y = 0;
    
    // Use Qt-compatible width calculation (works in both Qt 4 and 5)
    // Develop uses fm.horizontalAdvance() but we use fm.width() for broader compatibility
    int textWidth = fm.width(messageText);
    
    if (messageAlignment & Qt::AlignHCenter) {
        x = (widgetRect.width() - textWidth) / 2;
    } else if (messageAlignment & Qt::AlignRight) {
        x = widgetRect.width() - textWidth - 12;
    } else {
        x = 12; // Left align with margin (matching QSplashScreen default)
    }
    
    if (messageAlignment & Qt::AlignVCenter) {
        y = (widgetRect.height() + fm.height()) / 2;
    } else if (messageAlignment & Qt::AlignBottom) {
        y = widgetRect.height() - 12; // Bottom with margin (matching QSplashScreen default)
    } else {
        y = fm.height() + 12; // Top align with margin
    }
    
    // Draw main text in light green color (no glow effect)
    QColor lightGreen(144, 238, 144);
    painter.setPen(QPen(lightGreen, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawText(x, y, messageText);
}
