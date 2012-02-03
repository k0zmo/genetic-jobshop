#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "Evo.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), loaded(false)
{
    ui->setupUi(this);

    menuBar = new QMenuBar();
    fileMenu = menuBar->addMenu("File");

    readProblemAction = new QAction("Open problem file...", this);
    fileMenu->addAction(readProblemAction);

    quitAction = new QAction("Quit", this);
    fileMenu->addSeparator();
    fileMenu->addAction(quitAction);

    startAlgorithmAction = new QAction("Start algorithm", this);
    aboutAction = new QAction("About...", this);

    menuBar->addMenu(fileMenu);
    menuBar->addAction(startAlgorithmAction);
    menuBar->addAction(aboutAction);

    this->setWindowTitle("EA Project");
    this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    this->setMenuBar(menuBar);

	this->d_plot = new Plot(ui->graphWidget);
	//d_plot->resize(600, 400);
	d_plot->resize(661, 351);
	d_plot->show();

	// Dla lepszego "zwiedzania wykresu"
	{
		d_zoomer[0] = new Zoomer(QwtPlot::xBottom, QwtPlot::yLeft, d_plot->canvas());
		d_zoomer[0]->setRubberBand(QwtPicker::RectRubberBand);
		d_zoomer[0]->setRubberBandPen(QColor(Qt::green));
		d_zoomer[0]->setTrackerMode(QwtPicker::ActiveOnly);
		d_zoomer[0]->setTrackerPen(QColor(Qt::green));

		d_zoomer[1] = new Zoomer(QwtPlot::xTop, QwtPlot::yRight, d_plot->canvas());

		d_panner = new QwtPlotPanner(d_plot->canvas());
		d_panner->setMouseButton(Qt::MidButton);

		d_zoomer[0]->zoom(0);
		d_zoomer[1]->zoom(0);

		d_panner->setEnabled(true);
		d_zoomer[0]->setEnabled(true);
		d_zoomer[1]->setEnabled(true);
	}

    setDefaultValues();
    setValidators();
    connectSignalSlot();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::connectSignalSlot()
{
    // actions
    connect(this->aboutAction, SIGNAL(triggered()),
            this, SLOT(aboutActionHandler()));
    connect(this->readProblemAction, SIGNAL(triggered()),
            this, SLOT(readProblemActionHandler()));
    connect(this->quitAction, SIGNAL(triggered()),
            this, SLOT(quitActionHandler()));
    connect(this->startAlgorithmAction, SIGNAL(triggered()),
            this, SLOT(startAlgorithmActionHandler()));

    // widgets
    connect(ui->populationModelComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(populationModelChanged(int)));
    connect(ui->selectionSchemeComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(selectionMethodChanged(int)));
}

void MainWindow::setDefaultValues()
{
    // set default configuration fields values
    ui->populationSizeLineEdit->setText("500"); // default population size (500)
    ui->populationModelComboBox->setCurrentIndex(0); // pure reinsertion
    ui->selectionSchemeComboBox->setCurrentIndex(0); // uniform selection
    ui->fitnessEvaluationFunctionComboBox->setCurrentIndex(0); // LR
    ui->mutationRateLineEdit->setText("0.1"); // mutation rate
    ui->crossoverRateLineEdit->setText("0.5"); // crossover rate
    ui->rowColumnOperatorRateLineEdit->setText("0.5");
    ui->stopConditionComboBox->setCurrentIndex(0);

    ui->selectivePressureLineEdit->setText("2.0");
    ui->numberOfGenerationsLineEdit->setText("500");
    ui->allowDuplicatesCheckBox->setChecked(true);
    ui->tournamentGroupSizeLineEdit->setText("4");
    ui->genitorCheckBox->setChecked(true);
    ui->temporaryPopulationSizeLineEdit->setText("50");
    ui->reinsertionCoefficientLineEdit->setText("0.2");

    // set default widget visibility
    ui->tournamentSelGroupBox->setVisible(false);
    ui->temporaryPopulationSizeLabel->setVisible(false);
    ui->temporaryPopulationSizeLineEdit->setVisible(false);
    ui->reinsertionCoefficientLabel->setVisible(false);
    ui->reinsertionCoefficientLineEdit->setVisible(false);
}

void MainWindow::setValidators()
{
    QValidator *popSizeValidator = new QIntValidator(1, 10000, this);
    ui->populationSizeLineEdit->setValidator(popSizeValidator);
    ui->temporaryPopulationSizeLineEdit->setValidator(popSizeValidator);

    QDoubleValidator *mutCVRateValidator = new QDoubleValidator(this);
    mutCVRateValidator->setRange(0.0, 1.0, 3);
    ui->mutationRateLineEdit->setValidator(mutCVRateValidator);
    ui->crossoverRateLineEdit->setValidator(mutCVRateValidator);
    ui->rowColumnOperatorRateLineEdit->setValidator(mutCVRateValidator);
    ui->reinsertionCoefficientLineEdit->setValidator(mutCVRateValidator);
    ui->selectivePressureLineEdit->setValidator(mutCVRateValidator);

    QValidator *iterationsValidator = new QIntValidator(1, 10000000, this);
    ui->numberOfGenerationsLineEdit->setValidator(iterationsValidator);
}

void MainWindow::aboutActionHandler()
{
    QMessageBox::information(this, tr("Genetic algorithm JSP solver"),
                             tr("Application is a part of final project for Operations Research "
                                "laboratory.\n"
                                "Authors: Rafal Kulaga, Kajetan Swierk.\n"),
                             QMessageBox::Ok);
}

void MainWindow::readProblemActionHandler()
{
    QString pfName =
            QFileDialog::getOpenFileName(this, tr("Select Problem File"),
                                         QDir::currentPath(),
                                         tr("Problem files (*.dat)"));
    if (pfName.isEmpty())
        return;

    QFile pFile(pfName);
    if(!pFile.open(QFile::ReadOnly | QFile::Text))
    {
        QMessageBox::warning(this, tr("Genetic algorithm JSP solver"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(pfName)
                             .arg(pFile.errorString()));
        return;
    }

	fName = pfName;
    QTextStream pData(&pFile);
    pData.readLine();
    this->setProblemViewData(pData.readAll());
	loaded = true;
}

void MainWindow::startAlgorithmActionHandler()
{
	if(!loaded)
		return;

	ui->logBrowser->clear();

	enum EFinishCondition
	{
		FC_MAX_ITER,
		FC_WITHOUT_IMPROV
	};

	ui->tabWidget->setCurrentIndex(2);

	// HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
	std::string tmp = fName.toStdString();
	const char* filename = tmp.c_str();
	float selectivePressure = ui->selectivePressureLineEdit->text().toFloat();
	core::uint32 nTempPopSize = ui->temporaryPopulationSizeLineEdit->text().toUInt();
	core::uint32 nPopSize = ui->populationSizeLineEdit->text().toUInt();
	float replaceCoeff = ui->reinsertionCoefficientLineEdit->text().toFloat();
	float pMut = ui->mutationRateLineEdit->text().toFloat();
	float pCX = ui->crossoverRateLineEdit->text().toFloat();
	float pRowColumn = ui->rowColumnOperatorRateLineEdit->text().toFloat();
	bool genitor = ui->genitorCheckBox->isChecked();
	core::uint32 condExit = ui->numberOfGenerationsLineEdit->text().toUInt();
	core::uint32 tourGroupSize = ui->tournamentGroupSizeLineEdit->text().toUInt();
	bool allowDuplicates = ui->allowDuplicatesCheckBox->isChecked();

	Problem::EPopulationModel pm = static_cast<Problem::EPopulationModel>(ui->populationModelComboBox->currentIndex());
	Problem::ESelectionScheme ss = static_cast<Problem::ESelectionScheme>(ui->selectionSchemeComboBox->currentIndex());
	Problem::EFitnessModel fm = static_cast<Problem::EFitnessModel>(ui->fitnessEvaluationFunctionComboBox->currentIndex());
	EFinishCondition fc = static_cast<EFinishCondition>(ui->stopConditionComboBox->currentIndex());
	// HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH

	Problem problem;
	if(!problem.loadInitialData(filename))
	{
		QMessageBox::critical(0, "Exception occured!", "Cant find the problem file.");
		return;
	}
	problem.setPopulationModel(pm);
	problem.setProbability(pCX, pMut);
	problem.setSelectMethod(ss);
	problem.setFitnessModel(fm, selectivePressure);
	problem.setOperatorProbability(pRowColumn);
	problem.setGenitor(genitor);
	problem.setSSParameters(nTempPopSize, replaceCoeff, nPopSize);
	problem.generateRandomSolutions(nPopSize);
	problem.setTournamentParameters(tourGroupSize, allowDuplicates);

	QString popDsc;
	problem.getPopulationDesc(popDsc);

	// aktualizuj loga
	ui->logBrowser->append(popDsc);

	// wypisz najlepsze rozwiazanie losowe (tylko w celach porownawczych)
	problem.outputToMatlab("solution_init.m", 0);

	QVector<double> vmax;
	QVector<double> vmin;
	QVector<double> vavg;
	QVector<double> xaxis;

	// Inicjalizacja wektora danych wykresu
	{
		vmax.reserve(condExit);
		vmin.reserve(condExit);
		vavg.reserve(condExit);
		xaxis.reserve(condExit);

		vmax.push_back(static_cast<double>(problem.maxObjective));
		vmin.push_back(static_cast<double>(problem.minObjective));
		vavg.push_back(static_cast<double>(problem.average));
		xaxis.push_back(static_cast<double>(0));
	}

	// Do skalowania osi Y
	double vmax_max = vmax[0];
	double vmin_min = vmin[0];

	// Dane iteracji
	core::uint32 nIter = 0;
	core::uint32 nGenWithoutImprovements = 0;
	core::uint32 nLastBestGenObjScore = problem.minObjective;

	d_panner->setEnabled(false);
	d_zoomer[0]->setEnabled(false);
	d_zoomer[1]->setEnabled(false);

	// Glowna petla
	for(; ; ++nIter )
	{
		// Warunek zakonczenia dzialania algorytmu
		if(fc == FC_WITHOUT_IMPROV)
		{
			if(nGenWithoutImprovements >= condExit)
				break;
		}
		else
		{
			if(nIter >= condExit)
				break;
		}

		problem.nextGen();

		// Odswiezanie wykresu co 10 pokolen
		if(nIter % 100 == 0)
		{
			d_plot->setAxis(0.0f, static_cast<float>(nIter), vmin_min, vmax_max);
			d_plot->cMaxVal->setSamples(xaxis, vmax);
			d_plot->cMinVal->setSamples(xaxis, vmin);
			d_plot->cAvgVal->setSamples(xaxis, vavg);
			d_plot->replot();
		}

		// Zbieranie info o aktualnej populacji
		{
			QString iterDesc;
			QTextStream strm(&iterDesc);
			//QTextStream strm(stderr);
			strm.setRealNumberNotation(QTextStream::FixedNotation);
			strm.setRealNumberPrecision(3);
			strm << "Generation: " << nIter << "\t " << problem.minObjective << "/" 
				 << problem.maxObjective << "/" << problem.average << "/" << problem.stdDeviation << "[min/max/avg/stddev]\n";
			strm << "Generations without improvements: " << nGenWithoutImprovements << "\n";

			// aktualizuj loga
			ui->logBrowser->append(iterDesc);

			vmax.push_back(static_cast<double>(problem.maxObjective));
			vmin.push_back(static_cast<double>(problem.minObjective));
			vavg.push_back(static_cast<double>(problem.average));
			xaxis.push_back(static_cast<double>(nIter));

			vmax_max = std::max(vmax[nIter], vmax_max);
			vmin_min = std::min(vmin[nIter], vmin_min);

			// NOTE: bez GENITOR'a moga byc pogorszenia
			if(problem.minObjective >= nLastBestGenObjScore)
			{
				++nGenWithoutImprovements;
			}
			else
			{
				nGenWithoutImprovements = 0;
				nLastBestGenObjScore = problem.minObjective;
			}
		}
	}

	// Wyswietlenie w formie "verbose" wyniku koncowego algorytmu
	popDsc.clear();
	problem.getPopulationDesc(popDsc);
	// aktualizuj loga
	ui->logBrowser->append(popDsc);

	// Wypisanie najlepszego wyniku do pliku matlaba
	problem.outputToMatlab("solution_best.m", 0);

	// Narysowanie ostatecznego wykresu
	d_plot->setAxis(0.0f, static_cast<float>(nIter), vmin_min, vmax_max);
	d_plot->cMaxVal->setSamples(xaxis, vmax);
	d_plot->cMinVal->setSamples(xaxis, vmin);
	d_plot->cAvgVal->setSamples(xaxis, vavg);
	d_plot->replot();

	d_zoomer[0]->setZoomBase();
	d_zoomer[1]->setZoomBase();

	d_panner->setEnabled(true);
	d_zoomer[0]->setEnabled(true);
	d_zoomer[1]->setEnabled(true);
}

void MainWindow::quitActionHandler()
{
    this->close();
}

void MainWindow::setProblemViewData(const QString &data)
{
    ui->problemView->setPlainText(data);
    ui->tabWidget->setCurrentIndex(1);
}

void MainWindow::populationModelChanged(int idx)
{
    if(idx == 0) // pure reinsertion
    {
        ui->genitorCheckBox->setVisible(true);
        ui->genitorLabel->setVisible(true);

        ui->temporaryPopulationSizeLabel->setVisible(false);
        ui->temporaryPopulationSizeLineEdit->setVisible(false);
        ui->reinsertionCoefficientLabel->setVisible(false);
        ui->reinsertionCoefficientLineEdit->setVisible(false);

        return;
    }

    if(idx == 1 || idx == 2) // uniform reinsertion/elitist reinsertion
    {
        ui->temporaryPopulationSizeLabel->setVisible(true);
        ui->temporaryPopulationSizeLineEdit->setVisible(true);

        ui->genitorCheckBox->setVisible(false);
        ui->genitorLabel->setVisible(false);
        ui->reinsertionCoefficientLabel->setVisible(false);
        ui->reinsertionCoefficientLineEdit->setVisible(false);

        return;
    }

    if(idx == 3) // "4+3" population model (excess)
    {
        ui->temporaryPopulationSizeLabel->setVisible(true);
        ui->temporaryPopulationSizeLineEdit->setVisible(true);
        ui->reinsertionCoefficientLabel->setVisible(true);
        ui->reinsertionCoefficientLineEdit->setVisible(true);

        ui->genitorCheckBox->setVisible(false);
        ui->genitorLabel->setVisible(false);

        return;
    }
}

void MainWindow::selectionMethodChanged(int idx)
{
    if(idx == 2) // tournament selection method
    {
        ui->tournamentSelGroupBox->setVisible(true);
    }

    else
    {
        ui->tournamentSelGroupBox->setVisible(false);
    }
}
