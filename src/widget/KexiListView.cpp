KPageListView::KPageListView(QWidget *parent)
    : QListView(parent)
{
    setViewMode(QListView::ListMode);
    setMovement(QListView::Static);
    setVerticalScrollMode(QListView::ScrollPerPixel);

    QFont boldFont(font());
    boldFont.setBold(true);
    setFont(boldFont);

    setItemDelegate(new KPageListViewDelegate(this));
}

KPageListView::~KPageListView()
{
}

void KPageListView::setModel(QAbstractItemModel *model)
{
    /*
      KPageListViewProxy *proxy = new KPageListViewProxy( this );
      proxy->setSourceModel( model );
      proxy->rebuildMap();

      connect( model, SIGNAL(layoutChanged()), proxy, SLOT(rebuildMap()) );
    */
    connect(model, SIGNAL(layoutChanged()), this, SLOT(updateWidth()));

//  QListView::setModel( proxy );
    QListView::setModel(model);

    // Set our own selection model, which won't allow our current selection to be cleared
    setSelectionModel(new KDEPrivate::SelectionModel(model, this));

    updateWidth();
}

void KPageListView::updateWidth()
{
    if (!model()) {
        return;
    }

    int rows = model()->rowCount();

    int width = 0;
    for (int i = 0; i < rows; ++i) {
        width = qMax(width, sizeHintForIndex(model()->index(i, 0)).width());
    }

    setFixedWidth(width + 25);
}

// ----

KPageListViewDelegate::KPageListViewDelegate(QObject *parent)
    : QAbstractItemDelegate(parent)
{
}

static int layoutText(QTextLayout *layout, int maxWidth)
{
    qreal height = 0;
    int textWidth = 0;
    layout->beginLayout();
    while (true) {
        QTextLine line = layout->createLine();
        if (!line.isValid()) {
            break;
        }
        line.setLineWidth(maxWidth);
        line.setPosition(QPointF(0, height));
        height += line.height();
        textWidth = qMax(textWidth, qRound(line.naturalTextWidth() + 0.5));
    }
    layout->endLayout();
    return textWidth;
}

void KPageListViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid()) {
        return;
    }

    QStyleOptionViewItemV4 opt(option);
    opt.showDecorationSelected = true;
    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();

    int iconSize = style->pixelMetric(QStyle::PM_IconViewIconSize);
    const QString text = index.model()->data(index, Qt::DisplayRole).toString();
    const QIcon icon = index.model()->data(index, Qt::DecorationRole).value<QIcon>();
    const QPixmap pixmap = icon.pixmap(iconSize, iconSize);

    QFontMetrics fm = painter->fontMetrics();
    int wp = pixmap.width() / pixmap.devicePixelRatio();
    int hp = pixmap.height() / pixmap.devicePixelRatio();

    QTextLayout iconTextLayout(text, option.font);
    QTextOption textOption(Qt::AlignHCenter);
    iconTextLayout.setTextOption(textOption);
    int maxWidth = qMax(3 * wp, 8 * fm.height());
    layoutText(&iconTextLayout, maxWidth);

    QPen pen = painter->pen();
    QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
                              ? QPalette::Normal : QPalette::Disabled;
    if (cg == QPalette::Normal && !(option.state & QStyle::State_Active)) {
        cg = QPalette::Inactive;
    }

    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);
    if (option.state & QStyle::State_Selected) {
        painter->setPen(option.palette.color(cg, QPalette::HighlightedText));
    } else {
        painter->setPen(option.palette.color(cg, QPalette::Text));
    }

    painter->drawPixmap(option.rect.x() + (option.rect.width() / 2) - (wp / 2), option.rect.y() + 5, pixmap);
    if (!text.isEmpty()) {
        iconTextLayout.draw(painter, QPoint(option.rect.x() + (option.rect.width() / 2) - (maxWidth / 2), option.rect.y() + hp + 7));
    }

    painter->setPen(pen);

    drawFocus(painter, option, option.rect);
}

QSize KPageListViewDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QSize(0, 0);
    }

    QStyleOptionViewItemV4 opt(option);
    opt.showDecorationSelected = true;
    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();

    int iconSize = style->pixelMetric(QStyle::PM_IconViewIconSize);
    const QString text = index.model()->data(index, Qt::DisplayRole).toString();
    const QIcon icon = index.model()->data(index, Qt::DecorationRole).value<QIcon>();
    const QPixmap pixmap = icon.pixmap(iconSize, iconSize);

    QFontMetrics fm = option.fontMetrics;
    int gap = fm.height();
    int wp = pixmap.width() / pixmap.devicePixelRatio();
    int hp = pixmap.height() / pixmap.devicePixelRatio();

    if (hp == 0) {
        /**
         * No pixmap loaded yet, we'll use the default icon size in this case.
         */
        hp = iconSize;
        wp = iconSize;
    }

    QTextLayout iconTextLayout(text, option.font);
    int wt = layoutText(&iconTextLayout, qMax(3 * wp, 8 * fm.height()));
    int ht = iconTextLayout.boundingRect().height();

    int width, height;
    if (text.isEmpty()) {
        height = hp;
    } else {
        height = hp + ht + 10;
    }

    width = qMax(wt, wp) + gap;

    return QSize(width, height);
}

void KPageListViewDelegate::drawFocus(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect) const
{
    if (option.state & QStyle::State_HasFocus) {
        QStyleOptionFocusRect o;
        o.QStyleOption::operator=(option);
        o.rect = rect;
        o.state |= QStyle::State_KeyboardFocusChange;
        QPalette::ColorGroup cg = (option.state & QStyle::State_Enabled)
                                  ? QPalette::Normal : QPalette::Disabled;
        o.backgroundColor = option.palette.color(cg, (option.state & QStyle::State_Selected)
                            ? QPalette::Highlight : QPalette::Background);
        QApplication::style()->drawPrimitive(QStyle::PE_FrameFocusRect, &o, painter);
    }
}

/**
 * KPageListViewProxy
 */

KPageListViewProxy::KPageListViewProxy(QObject *parent)
    : QAbstractProxyModel(parent)
{
}

KPageListViewProxy::~KPageListViewProxy()
{
}

int KPageListViewProxy::rowCount(const QModelIndex &) const
{
    return mList.count();
}

int KPageListViewProxy::columnCount(const QModelIndex &) const
{
    return 1;
}

QModelIndex KPageListViewProxy::index(int row, int column, const QModelIndex &) const
{
    if (column > 1 || row >= mList.count()) {
        return QModelIndex();
    } else {
        return createIndex(row, column, mList[ row ].internalPointer());
    }
}

QModelIndex KPageListViewProxy::parent(const QModelIndex &) const
{
    return QModelIndex();
}

QVariant KPageListViewProxy::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.row() >= mList.count()) {
        return QVariant();
    }

    return sourceModel()->data(mList[ index.row() ], role);
}

QModelIndex KPageListViewProxy::mapFromSource(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    for (int i = 0; i < mList.count(); ++i) {
        if (mList[ i ] == index) {
            return createIndex(i, 0, index.internalPointer());
        }
    }

    return QModelIndex();
}

QModelIndex KPageListViewProxy::mapToSource(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    return mList[ index.row() ];
}

void KPageListViewProxy::rebuildMap()
{
    mList.clear();

    const QAbstractItemModel *model = sourceModel();
    if (!model) {
        return;
    }

    for (int i = 0; i < model->rowCount(); ++i) {
        addMapEntry(model->index(i, 0));
    }

    for (int i = 0; i < mList.count(); ++i) {
        qDebug("%d:0 -> %d:%d", i, mList[ i ].row(), mList[ i ].column());
    }

    emit layoutChanged();
}

void KPageListViewProxy::addMapEntry(const QModelIndex &index)
{
    if (sourceModel()->rowCount(index) == 0) {
        mList.append(index);
    } else {
        const int count = sourceModel()->rowCount(index);
        for (int i = 0; i < count; ++i) {
            addMapEntry(sourceModel()->index(i, 0, index));
        }
    }
}

SelectionModel::SelectionModel(QAbstractItemModel *model, QObject *parent)
    : QItemSelectionModel(model, parent)
{
}

void SelectionModel::clear()
{
    // Don't allow the current selection to be cleared
}

void SelectionModel::select(const QModelIndex &index, QItemSelectionModel::SelectionFlags command)
{
    // Don't allow the current selection to be cleared
    if (!index.isValid() && (command & QItemSelectionModel::Clear)) {
        return;
    }
    QItemSelectionModel::select(index, command);
}

void SelectionModel::select(const QItemSelection &selection, QItemSelectionModel::SelectionFlags command)
{
    // Don't allow the current selection to be cleared
    if (!selection.count() && (command & QItemSelectionModel::Clear)) {
        return;
    }
    QItemSelectionModel::select(selection, command);
}

