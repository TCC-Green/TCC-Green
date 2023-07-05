import os
import matplotlib.pyplot as plt
import numpy as np
from measure import *
from average import *

def printMeasures(values: AverageValues, tabs: int, unit: str, isLast = False, firstTab = "┃   "):
    tabsStr = ""
    if tabs == 3:
        if isLast is False:
            tabsStr = f"{firstTab}┃   "
        else:
            tabsStr = f"{firstTab}    "
    print(f"{tabsStr}┣━━ Min:    {values.min()}{unit}")
    print(f"{tabsStr}┣━━ Max:    {values.max()}{unit}")
    print(f"{tabsStr}┣━━ Avg:    {values.avg()}{unit}")
    print(f"{tabsStr}┗━━ StdDev: {values.stdDev()}{unit}")
    #print(f"{tabsStr}┣━━ StdErr: {values.stdErr()}{unit}")
    #print(f"{tabsStr}┗━━ 95% CI: {values.avg()} ± {values.confidenceInterval95()}{unit}")

def showMeasurements(result: ResultAverage):
    print(f"Algorithm: {getMeasureTypeName(result.baseResult.measures[1].type)}")
    print(f"Threads: {result.baseResult.threads}")
    print(f"Execution time (s):")
    printMeasures(result.executionTimeAverage, 1, "s")
    print(f"Energy (J):")
    print(f"┣━━ Idle:")
    print(f"┃   ┣━━ Pkg:")
    printMeasures(result.idleAverage.pkg, 3, "J")
    if result.allPackageAverage.pp0 is not None:
        print(f"┃   ┣━━ PP0:")
        printMeasures(result.idleAverage.pp0, 3, "J", isLast=(result.idleAverage.pp1 is None or result.idleAverage.dram is None))
    if result.allPackageAverage.pp1 is not None:
        print(f"┃   ┣━━ PP1:")
        printMeasures(result.idleAverage.pp1, 3, "J", isLast=(result.idleAverage.dram is None))
    if result.allPackageAverage.pp0 is not None:
        print(f"┃   ┗━━ DRAM:")
        printMeasures(result.idleAverage.dram, 3, "J", isLast=True)
    print(f"┣━━ All:")
    print(f"┃   ┣━━ Pkg:")
    printMeasures(result.allPackageAverage.pkg, 3, "J")
    if result.allPackageAverage.pp0 is not None:
        print(f"┃   ┣━━ PP0:")
        printMeasures(result.allPackageAverage.pp0, 3, "J", isLast=(result.allPackageAverage.pp1 is None or result.allPackageAverage.dram is None))
    if result.allPackageAverage.pp1 is not None:
        print(f"┃   ┣━━ PP1:")
        printMeasures(result.allPackageAverage.pp1, 3, "J", isLast=(result.allPackageAverage.dram is None))
    if result.allPackageAverage.pp0 is not None:
        print(f"┃   ┗━━ DRAM:")
        printMeasures(result.allPackageAverage.dram, 3, "J", isLast=True)
    pkgAvgValue: PackageAverage
    for pkgAvgValue in result.packageAverages:
        tabStr = "┃   "
        if pkgAvgValue.pkgNumber == len(result.packageAverages)-1:
            tabStr = "    "
            print(f"┗━━ Package {pkgAvgValue.pkgNumber}:")
        else:
            print(f"┣━━ Package {pkgAvgValue.pkgNumber}:")
        print(f"{tabStr}┣━━ Pkg:")
        printMeasures(pkgAvgValue.pkg, 3, "J", firstTab=tabStr)
        if pkgAvgValue.pp0 is not None:
            print(f"{tabStr}┣━━ PP0:")
            printMeasures(pkgAvgValue.pp0, 3, "J", isLast=(pkgAvgValue.pp1 is None or pkgAvgValue.dram is None), firstTab=tabStr)
        if pkgAvgValue.pp1 is not None:
            print(f"{tabStr}┣━━ PP1:")
            printMeasures(pkgAvgValue.pp1, 3, "J", isLast=(pkgAvgValue.dram is None), firstTab=tabStr)
        if pkgAvgValue.pp0 is not None:
            print(f"{tabStr}┗━━ DRAM:")
            printMeasures(pkgAvgValue.dram, 3, "J", isLast=True, firstTab=tabStr)

def showGraphSingle(graph: str, calculatedAvgs: ResultAverage) -> bool:
    graph = graph.lower()
    if graph == "jxs": # Energia (J) x Tempo (s) 
        valuesPkg = calculatedAvgs.allPackageAverage.pkg.values()
        valuesTime = calculatedAvgs.executionTimeAverage.values()

        for i in range(0, len(valuesPkg)):
            plt.plot(valuesTime[i], valuesPkg[i], ".", label=f"Teste {i+1}")
        
        executionTimeAvg = calculatedAvgs.executionTimeAverage.avg()
        allPackageAvg = calculatedAvgs.allPackageAverage.pkg.avg()
        executionTimeStdDev = calculatedAvgs.executionTimeAverage.stdDev()
        allPackageStdDev = calculatedAvgs.allPackageAverage.pkg.stdDev()
        plt.plot(executionTimeAvg, allPackageAvg, "o", label="Média", color="black")
        plt.errorbar(executionTimeAvg, allPackageAvg, xerr=executionTimeStdDev, yerr=allPackageStdDev)

        plt.xlabel("Tempo de execução (s)")
        plt.ylabel("Energia (J)")
        plt.legend()
        
        plt.show()
        return True
    return False

def hasDupes(calculatedAverages: list[ResultAverage]) -> bool:
    nameList: list[str] = []
    for calcAvg in calculatedAverages:
        name = f"{getMeasureTypeName(calcAvg.baseResult.measures[1].type)}{calcAvg.baseResult.threads}"
        if name in nameList:
            return True
        else:
            nameList.append(name)
    return False

def showGraphMultiple(graph: str, calculatedAvgs: list[ResultAverage]) -> bool:
    graph = graph.lower()
    params = {'legend.fontsize': 'x-large',
          'figure.figsize': (15, 5),
         'axes.labelsize': 'x-large',
         'axes.titlesize':'x-large',
         'xtick.labelsize':'x-large',
         'ytick.labelsize':'x-large'}
    plt.rcParams.update(params)
    if graph == "energy": # Energia (J)
        if hasDupes(calculatedAvgs):
            testNames: list[str] = []
            testValues: dict[str, list[float]] = {}
            #testNames: list[str] = ["Map", "Reduction", "Stencil"]
            #testValues: dict[str, list[float]] = {"O0": [1.0, 2.0, 3.0], "O1": [2.0, 3.0, 4.0], "O2": [3.0, 4.0, 5.0], "O3": [4.0, 5.0, 6.0]}
            #testErrors: list[float] = []

            for calcAvg in calculatedAvgs:
                vName = f"{getMeasureTypeName(calcAvg.baseResult.measures[1].type)} {calcAvg.baseResult.threads}T"
                if vName not in testNames:
                    testNames.append(vName)

            for calcAvg in calculatedAvgs:
                if f"{calcAvg.baseResult.threads}T" not in testValues:
                    floatList: list[float] = []
                    for i in range(0, len(testNames)):
                        floatList.append(0.0)
                    optimization = calcAvg.baseResult.fileName.split("-")[0]
                    testValues[optimization] = floatList

            for calcAvg in calculatedAvgs:
                vName = f"{getMeasureTypeName(calcAvg.baseResult.measures[1].type)} {calcAvg.baseResult.threads}T"
                idx = testNames.index(vName)
                optimization = calcAvg.baseResult.fileName.split("-")[0]
                testValues[optimization][idx] = calcAvg.allPackageAverage.pkg.avg()

            x = np.arange(len(testNames))  # the label locations
            width = 1.0/(len(testValues)+1)  # the width of the bars
            multiplier = 0

            fig, ax = plt.subplots(layout='constrained')

            for attribute, measurement in testValues.items():
                offset = width * multiplier
                rects = ax.bar(x + offset, measurement, width, label=attribute)
                ax.bar_label(rects, padding=5, fmt="{:,.02f}", rotation=90, fontsize="x-large")
                multiplier += 1

            ax.set_ylabel("Energia (J)")
            ax.set_xticks(x + (0.075 * len(testValues)), testNames)
            ax.legend()
            ax.set_ylim(0, ax.get_ylim()[1]*1.2)
        else:
            testNames: list[str] = []
            testValues: list[float] = []
            testErrors: list[float] = []

            for calcAvg in calculatedAvgs:
                testNames.append(f"{getMeasureTypeName(calcAvg.baseResult.measures[1].type)} - {calcAvg.baseResult.threads}T")
                testValues.append(calcAvg.allPackageAverage.pkg.avg())
                testErrors.append(calcAvg.allPackageAverage.pkg.stdDev())

            fig, ax = plt.subplots()
            bar_container = ax.bar(testNames, testValues)
            ax.errorbar(testNames, testValues, yerr=testErrors, color="black", fmt=".")
            ax.set_ylabel("Energia (J)")
            ax.bar_label(bar_container, fmt="{:,.02f}", rotation=90, fontsize="x-large")
            ax.set_ylim(0, ax.get_ylim()[1]*1.2)
        plt.show()
        return True
    if graph == "time": # Tempo (s)
        if hasDupes(calculatedAvgs):
            testNames: list[str] = []
            testValues: dict[str, list[float]] = {}

            for calcAvg in calculatedAvgs:
                vName = f"{getMeasureTypeName(calcAvg.baseResult.measures[1].type)} {calcAvg.baseResult.threads}T"
                if vName not in testNames:
                    testNames.append(vName)

            for calcAvg in calculatedAvgs:
                if f"{calcAvg.baseResult.threads}T" not in testValues:
                    floatList: list[float] = []
                    for i in range(0, len(testNames)):
                        floatList.append(0.0)
                    optimization = calcAvg.baseResult.fileName.split("-")[0]
                    testValues[optimization] = floatList

            for calcAvg in calculatedAvgs:
                vName = f"{getMeasureTypeName(calcAvg.baseResult.measures[1].type)} {calcAvg.baseResult.threads}T"
                idx = testNames.index(vName)
                optimization = calcAvg.baseResult.fileName.split("-")[0]
                testValues[optimization][idx] = calcAvg.executionTimeAverage.avg()

            x = np.arange(len(testNames))  # the label locations
            width = 1.0/(len(testValues)+1)  # the width of the bars
            multiplier = 0

            fig, ax = plt.subplots(layout='constrained')

            for attribute, measurement in testValues.items():
                offset = width * multiplier
                rects = ax.bar(x + offset, measurement, width, label=attribute)
                ax.bar_label(rects, padding=5, fmt="{:,.02f}", rotation=90, fontsize="x-large")
                multiplier += 1

            ax.set_ylabel("Tempo (s)")
            ax.set_xticks(x + (0.075 * len(testValues)), testNames)
            ax.legend()
            ax.set_ylim(0, ax.get_ylim()[1]*1.2)
        else:
            testNames: list[str] = []
            testValues: list[float] = []
            testErrors: list[float] = []

            for calcAvg in calculatedAvgs:
                testNames.append(f"{getMeasureTypeName(calcAvg.baseResult.measures[1].type)} - {calcAvg.baseResult.threads}T")
                testValues.append(calcAvg.executionTimeAverage.avg())
                testErrors.append(calcAvg.executionTimeAverage.stdDev())

            fig, ax = plt.subplots()
            bar_container = ax.bar(testNames, testValues)
            ax.errorbar(testNames, testValues, yerr=testErrors, color="black", fmt=".")
            ax.set_ylabel("Tempo (s)")
            ax.bar_label(bar_container, fmt="{:,.02f}", rotation=90, fontsize="x-large")
            ax.set_ylim(0, ax.get_ylim()[1]*1.2)
        plt.show()
        return True
    if graph == "j/s": # Tempo (s)
        if hasDupes(calculatedAvgs):
            testNames: list[str] = []
            testValues: dict[str, list[float]] = {}

            for calcAvg in calculatedAvgs:
                vName = f"{getMeasureTypeName(calcAvg.baseResult.measures[1].type)} {calcAvg.baseResult.threads}T"
                if vName not in testNames:
                    testNames.append(vName)

            for calcAvg in calculatedAvgs:
                if f"{calcAvg.baseResult.threads}T" not in testValues:
                    floatList: list[float] = []
                    for i in range(0, len(testNames)):
                        floatList.append(0.0)
                    optimization = calcAvg.baseResult.fileName.split("-")[0]
                    testValues[optimization] = floatList

            for calcAvg in calculatedAvgs:
                vName = f"{getMeasureTypeName(calcAvg.baseResult.measures[1].type)} {calcAvg.baseResult.threads}T"
                idx = testNames.index(vName)
                optimization = calcAvg.baseResult.fileName.split("-")[0]
                testValues[optimization][idx] = calcAvg.allPackageAverage.pkg.avg()/calcAvg.executionTimeAverage.avg()

            x = np.arange(len(testNames))  # the label locations
            width = 1.0/(len(testValues)+1)  # the width of the bars
            multiplier = 0

            fig, ax = plt.subplots(layout='constrained')

            for attribute, measurement in testValues.items():
                offset = width * multiplier
                rects = ax.bar(x + offset, measurement, width, label=attribute)
                ax.bar_label(rects, padding=5, fmt="{:,.02f}", rotation=90, fontsize="x-large")
                multiplier += 1

            ax.set_ylabel("Energia por Tempo (J/s)")
            ax.set_xticks(x + (0.075 * len(testValues)), testNames)
            ax.legend()
            ax.set_ylim(0, ax.get_ylim()[1]*1.2)
        else:
            testNames: list[str] = []
            testValues: list[float] = []

            for calcAvg in calculatedAvgs:
                testNames.append(f"{getMeasureTypeName(calcAvg.baseResult.measures[1].type)} - {calcAvg.baseResult.threads}T")
                testValues.append(calcAvg.allPackageAverage.pkg.avg()/calcAvg.executionTimeAverage.avg())

            fig, ax = plt.subplots()
            bar_container = ax.bar(testNames, testValues)
            ax.set_ylabel("Energia por Tempo (J/s)")
            ax.bar_label(bar_container, fmt="{:.,02f}", rotation=90, fontsize="x-large")
            ax.set_ylim(0, ax.get_ylim()[1]*1.2)
        plt.show()
        return True
    if graph == "mflops/w": # Mega Flops por Watt (MFlops/W)
        if hasDupes(calculatedAvgs):
            testNames: list[str] = []
            testValues: dict[str, list[float]] = {}

            for calcAvg in calculatedAvgs:
                vName = f"{getMeasureTypeName(calcAvg.baseResult.measures[1].type)} {calcAvg.baseResult.threads}T"
                if vName not in testNames:
                    testNames.append(vName)

            for calcAvg in calculatedAvgs:
                if f"{calcAvg.baseResult.threads}T" not in testValues:
                    floatList: list[float] = []
                    for i in range(0, len(testNames)):
                        floatList.append(0.0)
                    optimization = calcAvg.baseResult.fileName.split("-")[0]
                    testValues[optimization] = floatList

            for calcAvg in calculatedAvgs:
                vName = f"{getMeasureTypeName(calcAvg.baseResult.measures[1].type)} {calcAvg.baseResult.threads}T"
                idx = testNames.index(vName)
                optimization = calcAvg.baseResult.fileName.split("-")[0]
                flops = getMeasureTypeFlops(calcAvg.baseResult.measures[1].type, calcAvg.baseResult.arraySize, calcAvg.baseResult.iterations) / calcAvg.executionTimeAverage.avg()
                mflops = flops/1000000
                watt = calcAvg.allPackageAverage.pkg.avg()/calcAvg.executionTimeAverage.avg()
                mflopswatt = mflops/watt
                testValues[optimization][idx] = mflopswatt

            x = np.arange(len(testNames))  # the label locations
            width = 1.0/(len(testValues)+1)  # the width of the bars
            multiplier = 0

            fig, ax = plt.subplots(layout='constrained')

            for attribute, measurement in testValues.items():
                offset = width * multiplier
                rects = ax.bar(x + offset, measurement, width, label=attribute)
                ax.bar_label(rects, padding=5, fmt="{:,.02f}", rotation=90, fontsize="x-large")
                multiplier += 1

            ax.set_ylabel("Megaflop por Watt (MFlop/W)")
            ax.set_xticks(x + (0.075 * len(testValues)), testNames)
            ax.legend()
            ax.set_ylim(0, ax.get_ylim()[1]*1.2)
        else:
            testNames: list[str] = []
            testValues: list[float] = []

            for calcAvg in calculatedAvgs:
                testNames.append(f"{getMeasureTypeName(calcAvg.baseResult.measures[1].type)} - {calcAvg.baseResult.threads}T")
                flops = getMeasureTypeFlops(calcAvg.baseResult.measures[1].type, calcAvg.baseResult.arraySize, calcAvg.baseResult.iterations) / calcAvg.executionTimeAverage.avg()
                mflops = flops/1000000
                watt = calcAvg.allPackageAverage.pkg.avg()/calcAvg.executionTimeAverage.avg()
                mflopswatt = mflops/watt
                testValues.append(mflopswatt)

            fig, ax = plt.subplots()
            bar_container = ax.bar(testNames, testValues)
            ax.set_ylabel("Megaflop por Watt (MFlop/W)")
            ax.bar_label(bar_container, fmt="{:,.02f}", rotation=90, fontsize="x-large")
            ax.set_ylim(0, ax.get_ylim()[1]*1.2)
        plt.show()
        return True
    return False

print("Input 'close' to close and 'return' to return to the previous step.")
folder: str | None = None
file: str | list[str] | None = None
close = False
analyzedFile: Result | list[Result] | None = None
calculatedAverages: ResultAverage | list[ResultAverage] | None = None
while (not close):
    if folder is not None:
        # analyze file and pick graph
        if file is not None:
            # show graph
            if isinstance(file, list):
                # multiple files
                print("Graphs: energy, time, j/s, mflops/w")
                inputGraph = input("Pick a graph: ")
                if inputGraph == "close" or inputGraph == "exit":
                    close = True
                elif inputGraph == "return" or inputGraph == "r" or inputGraph == "back" or inputGraph == "b":
                    file = None
                elif showGraphMultiple(inputGraph, calculatedAverages):
                    pass
                else:
                    print("Unknown graph")
            else:
                # single file
                showMeasurements(calculatedAverages)
                print("Graphs: energy, time, j/s, mflops/w")
                inputGraph = input("Pick a graph: ")
                if inputGraph == "close" or inputGraph == "exit":
                    close = True
                elif inputGraph == "return" or inputGraph == "r" or inputGraph == "back" or inputGraph == "b":
                    file = None
                elif showGraphSingle(inputGraph, calculatedAverages):
                    pass
                else:
                    print("Unknown graph")
        # select file
        else:
            print()
            print("Files: ", end="")
            for dirFile in os.listdir(folder):
                if os.path.isfile(os.path.join(folder, dirFile)):
                    print(dirFile, end=" ")
            print()
            inputFile = input("File to analyze: ")
            if inputFile.find(" ") != -1:
                inputFile = inputFile.split(" ")
                if any(not os.path.isfile(os.path.join(folder, inputFileVar)) for inputFileVar in inputFile):
                    print("Unknown file")
                else:
                    file = inputFile
                    analyzedFile = []
                    calculatedAverages = []
                    for fileVar in file:
                        analyzedFileVar = analyzeFile(os.path.join(folder, fileVar))
                        analyzedFile.append(analyzedFileVar)
                        calculatedAverages.append(calculateAverages(analyzedFileVar))
            else:
                if os.path.isfile(os.path.join(folder, inputFile)):
                    file = inputFile
                    analyzedFile = analyzeFile(os.path.join(folder, file))
                    calculatedAverages = calculateAverages(analyzedFile)
                else:
                    if inputFile == "close" or inputFile == "exit":
                        close = True
                    elif inputFile == "return" or inputFile == "r" or inputFile == "back" or inputFile == "b":
                        folder = None
                    else:
                        print("Unknown file")
    # select folder
    else:
        print()
        print("Folders: ", end="")
        for dir in os.listdir("."):
            if os.path.isdir(dir) and not dir.startswith("__"):
                print(dir, end=" ")
        print()
        inputFolder = input("Folder name: ")
        if os.path.isdir(inputFolder) and not inputFolder.startswith("__"):
            folder = inputFolder
        else:
            if inputFolder == "close" or inputFolder == "exit":
                close = True
            elif inputFolder == "return" or inputFolder == "r" or inputFolder == "back" or inputFolder == "b":
                print("Can't return, already at start point")
            else:
                print("Unknown folder")