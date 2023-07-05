import math
from typing import List
from measure import *

class AverageValues:
    def __init__(self) -> None:
        self.__values = []

    def values(self) -> List[float]:
        return self.__values

    def add(self, value: float):
        self.__values.append(round(value, 4))

    def min(self) -> float:
        minValue = None
        for i in range(0, len(self.__values)):
            if minValue is None or minValue > self.__values[i]:
                minValue = self.__values[i]
        return minValue
    
    def max(self) -> float:
        maxValue = None
        for i in range(0, len(self.__values)):
            if maxValue is None or maxValue < self.__values[i]:
                maxValue = self.__values[i]
        return maxValue
    
    def avg(self) -> float:
        avgValue = 0.0
        for i in range(0, len(self.__values)):
            avgValue += self.__values[i]
        return round(avgValue / len(self.__values), 4)
    
    def stdDev(self) -> float:
        avgValue = self.avg()
        sum = 0.0
        for i in range(0, len(self.__values)):
            sum += pow(self.__values[i] - avgValue, 2)
        return round(math.sqrt(sum / len(self.__values)), 4)
    
    def stdErr(self) -> float:
        return round(self.stdDev() / math.sqrt(len(self.__values)), 4)
    
    def confidenceInterval95(self) -> float:
        return round(1.96*self.stdErr(), 4)

class PackageAverage:
    def __init__(self) -> None:
        self.pkgNumber = None
        self.pkg = AverageValues()
        self.pp0 = None
        self.pp1 = None
        self.dram = None

class ResultAverage:
    def __init__(self) -> None:
        self.baseResult: Result | None = None
        self.executionTimeAverage = AverageValues()
        self.idleAverage = PackageAverage()
        self.packageAverages: List[PackageAverage] = []
        self.allPackageAverage = PackageAverage()

def addToAvg(value: float, avg: AverageValues):
    avg.add(value)

def addPackageValue(measure: PackageMeasure | list[PackageMeasure], pkgAvg: PackageAverage):
    if isinstance(measure, list): # especifico para o AllPackages (logo não há um pkgNumber)
        pkgAvg.pkgNumber = -1
        pkgAvg.pkg.add(sum(m.pkg for m in measure))
        if any(m.pp0 is not None for m in measure):
            pkgAvg.pp0 = AverageValues()
            pkgAvg.pp0.add(sum(m.pp0 if m.pp0 is not None else 0 for m in measure))
        if any(m.pp1 is not None for m in measure):
            pkgAvg.pp1 = AverageValues()
            pkgAvg.pp1.add(sum(m.pp1 if m.pp1 is not None else 0 for m in measure))
        if any(m.dram is not None for m in measure):
            pkgAvg.dram = AverageValues()
            pkgAvg.dram.add(sum(m.dram if m.dram is not None else 0 for m in measure))
    else:
        pkgAvg.pkgNumber = measure.pkgNumber
        pkgAvg.pkg.add(measure.pkg)
        if measure.pp0 is not None:
            if pkgAvg.pp0 is None:
                pkgAvg.pp0 = AverageValues()
            pkgAvg.pp0.add(measure.pp0)
        if measure.pp1 is not None:
            if pkgAvg.pp1 is None:
                pkgAvg.pp1 = AverageValues()
            pkgAvg.pp1.add(measure.pp1)
        if measure.dram is not None:
            if pkgAvg.dram is None:
                pkgAvg.dram = AverageValues()
            pkgAvg.dram.add(measure.dram)

def calculateAverages(result: Result) -> ResultAverage:
    allAvg = ResultAverage()

    packageCount = 0
    measure: Measure
    pkgMeasure: PackageMeasure
    for measure in result.measures:
        for pkgMeasure in measure.packages:
            if pkgMeasure.pkgNumber > packageCount:
                packageCount = pkgMeasure.pkgNumber
    packageCount += 1
    for i in range(0, packageCount):
        allAvg.packageAverages.append(PackageAverage())

    allAvg.baseResult = result
    for measure in result.measures:
        if measure.type == MeasureType.IDLE:
            for pkgMeasure in measure.packages:
                addPackageValue(pkgMeasure, allAvg.idleAverage)
        else:
            addToAvg(measure.executionTime, allAvg.executionTimeAverage)
            addPackageValue(measure.packages, allAvg.allPackageAverage)
            for pkgMeasure in measure.packages:
                addPackageValue(pkgMeasure, allAvg.packageAverages[pkgMeasure.pkgNumber])
    
    return allAvg