/***************************************************************************
                         qgsnewmemorylayerdialog.cpp
                             -------------------
    begin                : September 2014
    copyright            : (C) 2014 by Nyall Dawson, Marco Hugentobler
    email                : nyall dot dawson at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsnewmemorylayerdialog.h"
#include "qgsapplication.h"
#include "qgis.h"
#include "qgscoordinatereferencesystem.h"
#include "qgsproviderregistry.h"
#include "qgsvectordataprovider.h"
#include "qgsvectorlayer.h"

#include <QPushButton>
#include <QComboBox>
#include <QLibrary>
#include <QSettings>
#include <QUuid>
#include <QFileDialog>

QgsVectorLayer *QgsNewMemoryLayerDialog::runAndCreateLayer( QWidget *parent )
{
  QgsNewMemoryLayerDialog dialog( parent );
  if ( dialog.exec() == QDialog::Rejected )
  {
    return nullptr;
  }

  QgsWkbTypes::Type geometrytype = dialog.selectedType();

  QString geomType;
  switch ( geometrytype )
  {
    case QgsWkbTypes::Point:
      geomType = QStringLiteral( "point" );
      break;
    case QgsWkbTypes::LineString:
      geomType = QStringLiteral( "linestring" );
      break;
    case QgsWkbTypes::Polygon:
      geomType = QStringLiteral( "polygon" );
      break;
    case QgsWkbTypes::MultiPoint:
      geomType = QStringLiteral( "multipoint" );
      break;
    case QgsWkbTypes::MultiLineString:
      geomType = QStringLiteral( "multilinestring" );
      break;
    case QgsWkbTypes::MultiPolygon:
      geomType = QStringLiteral( "multipolygon" );
      break;
    case QgsWkbTypes::NoGeometry:
      geomType = QStringLiteral( "none" );
      break;
    default:
      geomType = QStringLiteral( "point" );
  }

  QString layerProperties = QStringLiteral( "%1?" ).arg( geomType );
  if ( QgsWkbTypes::NoGeometry != geometrytype )
    layerProperties.append( QStringLiteral( "crs=%1&" ).arg( dialog.crs().authid() ) );
  layerProperties.append( QStringLiteral( "memoryid=%1" ).arg( QUuid::createUuid().toString() ) );

  QString name = dialog.layerName().isEmpty() ? tr( "New scratch layer" ) : dialog.layerName();
  QgsVectorLayer* newLayer = new QgsVectorLayer( layerProperties, name, QStringLiteral( "memory" ) );
  return newLayer;
}

QgsNewMemoryLayerDialog::QgsNewMemoryLayerDialog( QWidget *parent, Qt::WindowFlags fl )
    : QDialog( parent, fl )
{
  setupUi( this );

  QSettings settings;
  restoreGeometry( settings.value( QStringLiteral( "/Windows/NewMemoryLayer/geometry" ) ).toByteArray() );

  mPointRadioButton->setChecked( true );

  QgsCoordinateReferenceSystem defaultCrs = QgsCoordinateReferenceSystem::fromOgcWmsCrs( settings.value( QStringLiteral( "/Projections/layerDefaultCrs" ), GEO_EPSG_CRS_AUTHID ).toString() );
  defaultCrs.validate();
  mCrsSelector->setCrs( defaultCrs );

  mNameLineEdit->setText( tr( "New scratch layer" ) );
}

QgsNewMemoryLayerDialog::~QgsNewMemoryLayerDialog()
{
  QSettings settings;
  settings.setValue( QStringLiteral( "/Windows/NewMemoryLayer/geometry" ), saveGeometry() );
}

QgsWkbTypes::Type QgsNewMemoryLayerDialog::selectedType() const
{
  if ( !buttonGroupGeometry->isChecked() )
  {
    return QgsWkbTypes::NoGeometry;
  }
  else if ( mPointRadioButton->isChecked() )
  {
    return QgsWkbTypes::Point;
  }
  else if ( mLineRadioButton->isChecked() )
  {
    return QgsWkbTypes::LineString;
  }
  else if ( mPolygonRadioButton->isChecked() )
  {
    return QgsWkbTypes::Polygon;
  }
  else if ( mMultiPointRadioButton->isChecked() )
  {
    return QgsWkbTypes::MultiPoint;
  }
  else if ( mMultiLineRadioButton->isChecked() )
  {
    return QgsWkbTypes::MultiLineString;
  }
  else if ( mMultiPolygonRadioButton->isChecked() )
  {
    return QgsWkbTypes::MultiPolygon;
  }
  return QgsWkbTypes::Unknown;
}

QgsCoordinateReferenceSystem QgsNewMemoryLayerDialog::crs() const
{
  return mCrsSelector->crs();
}

QString QgsNewMemoryLayerDialog::layerName() const
{
  return mNameLineEdit->text();
}
