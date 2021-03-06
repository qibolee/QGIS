# -*- coding: utf-8 -*-

"""
***************************************************************************
    RasterLayerHistogram.py
    ---------------------
    Date                 : January 2013
    Copyright            : (C) 2013 by Victor Olaya
    Email                : volayaf at gmail dot com
***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************
"""
from builtins import str
from builtins import range

__author__ = 'Victor Olaya'
__date__ = 'January 2013'
__copyright__ = '(C) 2013, Victor Olaya'

# This will get replaced with a git SHA1 when you do a git archive

__revision__ = '$Format:%H$'

import matplotlib.pyplot as plt
import matplotlib.pylab as lab

from qgis.PyQt.QtCore import QVariant
from qgis.core import QgsField

from processing.core.GeoAlgorithm import GeoAlgorithm
from processing.core.parameters import ParameterNumber
from processing.core.parameters import ParameterRaster
from processing.core.outputs import OutputTable
from processing.core.outputs import OutputHTML
from processing.tools import dataobjects
from processing.tools import raster


class RasterLayerHistogram(GeoAlgorithm):

    INPUT = 'INPUT'
    PLOT = 'PLOT'
    TABLE = 'TABLE'
    BINS = 'BINS'

    def defineCharacteristics(self):
        self.name, self.i18n_name = self.trAlgorithm('Raster layer histogram')
        self.group, self.i18n_group = self.trAlgorithm('Graphics')

        self.addParameter(ParameterRaster(self.INPUT,
                                          self.tr('Input layer')))
        self.addParameter(ParameterNumber(self.BINS,
                                          self.tr('Number of bins'), 2, None, 10))

        self.addOutput(OutputHTML(self.PLOT, self.tr('Histogram')))
        self.addOutput(OutputTable(self.TABLE, self.tr('Table')))

    def processAlgorithm(self, progress):
        layer = dataobjects.getObjectFromUri(
            self.getParameterValue(self.INPUT))
        nbins = self.getParameterValue(self.BINS)

        outputplot = self.getOutputValue(self.PLOT)
        outputtable = self.getOutputFromName(self.TABLE)

        values = raster.scanraster(layer, progress)

        # ALERT: this is potentially blocking if the layer is too big
        plt.close()
        valueslist = []
        for v in values:
            if v is not None:
                valueslist.append(v)
        (n, bins, values) = plt.hist(valueslist, nbins)

        fields = [QgsField('CENTER_VALUE', QVariant.Double),
                  QgsField('NUM_ELEM', QVariant.Double)]
        writer = outputtable.getTableWriter(fields)
        for i in range(len(values)):
            writer.addRecord([str(bins[i]) + '-' + str(bins[i + 1]), n[i]])

        plotFilename = outputplot + '.png'
        lab.savefig(plotFilename)
        with open(outputplot, 'w') as f:
            f.write('<html><img src="' + plotFilename + '"/></html>')
