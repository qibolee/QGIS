/***************************************************************************
  qgsattributeactionpropertiesdialog.cpp - QgsAttributeActionPropertiesDialog

 ---------------------
 begin                : 18.4.2016
 copyright            : (C) 2016 by Matthias Kuhn
 email                : matthias@opengis.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsattributeactionpropertiesdialog.h"
#include "qgsfieldexpressionwidget.h"
#include "qgisapp.h"
#include "qgsmapcanvas.h"
#include "qgsproject.h"
#include "qgsvectorlayer.h"
#include "qgsapplication.h"
#include "qgsactionscoperegistry.h"
#include "qgsactionscope.h"

#include <QComboBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QFileDialog>
#include <QImageWriter>

QgsAttributeActionPropertiesDialog::QgsAttributeActionPropertiesDialog( QgsAction::ActionType type, const QString& description, const QString& shortTitle, const QString& iconPath, const QString& actionText, bool capture, QSet<QString> actionScopes, QgsVectorLayer* layer, QWidget* parent )
    : QDialog( parent )
    , mLayer( layer )
{
  setupUi( this );

  mActionType->setCurrentIndex( type );
  mActionName->setText( description );
  mShortTitle->setText( shortTitle );
  mActionIcon->setText( iconPath );
  mIconPreview->setPixmap( QPixmap( iconPath ) );
  mActionText->setText( actionText );
  mCaptureOutput->setChecked( capture );

  init( actionScopes );
}

QgsAttributeActionPropertiesDialog::QgsAttributeActionPropertiesDialog( QgsVectorLayer* layer, QWidget* parent )
    : QDialog( parent )
    , mLayer( layer )
{
  setupUi( this );

  QSet<QString> defaultActionScopes;
  defaultActionScopes << QStringLiteral( "Canvas" )
  << QStringLiteral( "FieldSpecific" )
  << QStringLiteral( "AttributeTableRow" )
  << QStringLiteral( "FeatureForm" );

  init( defaultActionScopes );
}

QgsAction::ActionType QgsAttributeActionPropertiesDialog::type() const
{
  return static_cast<QgsAction::ActionType>( mActionType->currentIndex() );
}

QString QgsAttributeActionPropertiesDialog::description() const
{
  return mActionName->text();
}

QString QgsAttributeActionPropertiesDialog::shortTitle() const
{
  return mShortTitle->text();
}

QString QgsAttributeActionPropertiesDialog::iconPath() const
{
  return mActionIcon->text();
}

QString QgsAttributeActionPropertiesDialog::actionText() const
{
  return mActionText->text();
}

QSet<QString> QgsAttributeActionPropertiesDialog::actionScopes() const
{
  QSet<QString> actionScopes;

  Q_FOREACH ( QCheckBox* cb, mActionScopeCheckBoxes )
  {
    if ( cb->isChecked() )
      actionScopes.insert( cb->property( "ActionScopeName" ).toString() );
  }

  return actionScopes;
}

bool QgsAttributeActionPropertiesDialog::capture() const
{
  return mCaptureOutput->isChecked();
}

QgsExpressionContext QgsAttributeActionPropertiesDialog::createExpressionContext() const
{
  QgsExpressionContext context = mLayer->createExpressionContext();

  Q_FOREACH ( QCheckBox* cb, mActionScopeCheckBoxes )
  {
    if ( cb->isChecked() )
    {
      QgsActionScope actionScope = QgsApplication::actionScopeRegistry()->actionScope( cb->property( "ActionScopeName" ).toString() );
      context.appendScope( new QgsExpressionContextScope( actionScope.expressionContextScope() ) );
    }
  }

  return context;
}

void QgsAttributeActionPropertiesDialog::browse()
{
  // Popup a file browser and place the results into the action widget
  QString action = QFileDialog::getOpenFileName(
                     this, tr( "Select an action", "File dialog window title" ), QDir::homePath() );

  if ( !action.isNull() )
    mActionText->insertText( action );
}

void QgsAttributeActionPropertiesDialog::insertExpressionOrField()
{
  QString selText = mActionText->selectedText();

  // edit the selected expression if there's one
  if ( selText.startsWith( QLatin1String( "[%" ) ) && selText.endsWith( QLatin1String( "%]" ) ) )
    selText = selText.mid( 2, selText.size() - 4 );

  mActionText->insertText( "[%" + mFieldExpression->currentField() + "%]" );
}

void QgsAttributeActionPropertiesDialog::chooseIcon()
{
  QList<QByteArray> list = QImageWriter::supportedImageFormats();
  QStringList formatList;
  Q_FOREACH ( const QByteArray& format, list )
    formatList << QStringLiteral( "*.%1" ).arg( QString( format ) );

  QString filter = tr( "Images( %1 ); All( *.* )" ).arg( formatList.join( QStringLiteral( " " ) ) );
  QString icon = QFileDialog::getOpenFileName( this, tr( "Choose Icon..." ), mActionIcon->text(), filter );

  if ( !icon.isNull() )
  {
    mActionIcon->setText( icon );
    mIconPreview->setPixmap( QPixmap( icon ) );
  }
}

void QgsAttributeActionPropertiesDialog::updateButtons()
{
  if ( mActionName->text().isEmpty() || mActionText->text().isEmpty() )
  {
    mButtonBox->button( QDialogButtonBox::Ok )->setEnabled( false );
  }
  else
  {
    mButtonBox->button( QDialogButtonBox::Ok )->setEnabled( true );
  }
}

void QgsAttributeActionPropertiesDialog::init( const QSet<QString>& actionScopes )
{
  QSet<QgsActionScope> availableActionScopes = QgsApplication::actionScopeRegistry()->actionScopes();

  Q_FOREACH ( const QgsActionScope& scope, availableActionScopes )
  {
    QCheckBox* actionScopeCheckBox = new QCheckBox( scope.title() );
    if ( actionScopes.contains( scope.id() ) )
      actionScopeCheckBox->setChecked( true );
    QStringList variables = scope.expressionContextScope().variableNames();

    QString tooltip = scope.description();
    if ( !variables.empty() )
    {
      tooltip += "<br><br>";
      tooltip += tr( "Additional variables" );
      tooltip += "<ul><li>";
      tooltip += variables.join( "</li><li>" );
      tooltip += "</ul></li>";
    }
    actionScopeCheckBox->setToolTip( tooltip );
    actionScopeCheckBox->setProperty( "ActionScopeName", scope.id() );
    mActionScopeCheckBoxes.append( actionScopeCheckBox );
    mActionScopesGroupBox->layout()->addWidget( actionScopeCheckBox );
  }

  QgsDistanceArea myDa;
  myDa.setSourceCrs( mLayer->crs().srsid() );
  myDa.setEllipsoidalMode( QgisApp::instance()->mapCanvas()->mapSettings().hasCrsTransformEnabled() );
  myDa.setEllipsoid( QgsProject::instance()->ellipsoid() );

  mFieldExpression->setLayer( mLayer );
  mFieldExpression->setGeomCalculator( myDa );
  mFieldExpression->registerExpressionContextGenerator( this );

  connect( mBrowseButton, SIGNAL( clicked( bool ) ), this, SLOT( browse() ) );
  connect( mInsertFieldOrExpression, SIGNAL( clicked( bool ) ), this, SLOT( insertExpressionOrField() ) );
  connect( mActionName, SIGNAL( textChanged( QString ) ), this, SLOT( updateButtons() ) );
  connect( mActionText, SIGNAL( textChanged() ), this, SLOT( updateButtons() ) );
  connect( mChooseIconButton, SIGNAL( clicked( bool ) ), this, SLOT( chooseIcon() ) );

  updateButtons();
}
