#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_curve.h>
#include <qwt_legend.h>
#include <qwt_series_data.h>
#include <qwt_text.h>
#include <qwt_math.h>

#include <qwt_picker_machine.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_renderer.h>

class Zoomer: public QwtPlotZoomer
{
public:
	Zoomer(int xAxis, int yAxis, QwtPlotCanvas *canvas):
	  QwtPlotZoomer(xAxis, yAxis, canvas)
	  {
		  setTrackerMode(QwtPicker::AlwaysOff);
		  setRubberBand(QwtPicker::NoRubberBand);

		  // RightButton: zoom out by 1
		  // Ctrl+RightButton: zoom out to full size

		  setMousePattern(QwtEventPattern::MouseSelect2,
			  Qt::RightButton, Qt::ControlModifier);
		  setMousePattern(QwtEventPattern::MouseSelect3,
			  Qt::RightButton);
	  }
};

class Plot : public QwtPlot
{
public:
	QwtPlotCurve* cMaxVal;
	QwtPlotCurve* cMinVal;
	QwtPlotCurve* cAvgVal;

	Plot(QWidget* parent = NULL)
		: QwtPlot(parent)
	{
		setTitle("Scope");
		insertLegend(new QwtLegend(), QwtPlot::RightLegend);

		// Set axes
		setAxisTitle(xBottom, "Generation");
		setAxisTitle(yLeft, "Objective score");

		setCanvasBackground(Qt::white);

		setAxis(0, 50, 0, 100);

		cMaxVal = new QwtPlotCurve("Max obj. score");
		cMinVal = new QwtPlotCurve("Min obj. score");
		cAvgVal = new QwtPlotCurve("Avg obj. score");

		cMaxVal->setStyle(QwtPlotCurve::Steps);
		cMaxVal->setCurveAttribute(QwtPlotCurve::Inverted);
		cMaxVal->setRenderHint(QwtPlotItem::RenderAntialiased);
		cMaxVal->setLegendAttribute(QwtPlotCurve::LegendShowLine, true);
		cMaxVal->setPen(QPen(Qt::red));
		cMaxVal->attach(this);

		cMinVal->setStyle(QwtPlotCurve::Steps);
		cMinVal->setCurveAttribute(QwtPlotCurve::Inverted);
		cMinVal->setRenderHint(QwtPlotItem::RenderAntialiased);
		cMinVal->setLegendAttribute(QwtPlotCurve::LegendShowLine, true);
		cMinVal->setPen(QPen(Qt::green));
		cMinVal->attach(this);

		cAvgVal->setStyle(QwtPlotCurve::Steps);
		cAvgVal->setCurveAttribute(QwtPlotCurve::Inverted);
		cAvgVal->setRenderHint(QwtPlotItem::RenderAntialiased);
		cAvgVal->setLegendAttribute(QwtPlotCurve::LegendShowLine, true);
		cAvgVal->setPen(QPen(Qt::blue));
		cAvgVal->attach(this);

		// Insert markers

		//  ...a horizontal line at y = 0...
		QwtPlotMarker *mY = new QwtPlotMarker();
		mY->setLabel(QString::fromLatin1(""));
		mY->setLabelAlignment(Qt::AlignRight|Qt::AlignTop);
		mY->setLineStyle(QwtPlotMarker::HLine);
		mY->setLinePen(QPen(Qt::white));
		mY->setYValue(0.0);
		mY->attach(this);

		//  ...and vertical line at x = 0...
		QwtPlotMarker *mX = new QwtPlotMarker();
		mX->setLabel(QString::fromLatin1(""));
		mX->setLabelAlignment(Qt::AlignRight|Qt::AlignTop);
		mX->setLineStyle(QwtPlotMarker::VLine);
		mX->setLinePen(QPen(Qt::white));
		mX->setYValue(0.0);
		mX->attach(this);
	}

	void setAxis(float xmin, float xmax, float ymin, float ymax)
	{
		setAxisScale(xBottom, xmin, xmax);
		setAxisScale(yLeft, ymin, ymax);
	}
};