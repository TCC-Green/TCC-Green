import pathlib

class MeasureType:
    IDLE = 0
    MAP = 1
    REDUCTION = 2
    STENCIL = 3

class PackageMeasure:
    def __init__(self) -> None:
        self.pkgNumber: int = -1
        self.pkg: float | None = None
        self.pp0: float | None = None
        self.pp1: float | None = None
        self.dram: float | None = None

class Measure:
    def __init__(self) -> None:
        self.type: MeasureType = MeasureType.IDLE
        self.executionTime: float = 0.0
        self.packages: list[PackageMeasure] = []

class Result:
    def __init__(self) -> None:
        self.fileName: str | None = None
        self.threads: int = -1
        self.iterations: int = -1
        self.arraySize: int = -1
        self.measures: list[Measure] = []

def getMeasureTypeName(measureType: MeasureType) -> str:
    match measureType:
        case MeasureType.MAP:
            return "Map"
        case MeasureType.REDUCTION:
            return "Reduction"
        case MeasureType.STENCIL:
            return "Stencil"
        case MeasureType.IDLE:
            return "Idle"
        case _:
            return "Unknown"

def getMeasureTypeFlops(measureType: MeasureType, arrSize: int, iterations: int) -> float:
    match measureType:
        case MeasureType.MAP:
            return arrSize * arrSize * iterations * 2 # 1 add, 1 mul
        case MeasureType.REDUCTION:
            return arrSize * arrSize * iterations * 1 # 1 add
        case MeasureType.STENCIL:
            return arrSize * arrSize * iterations * 4 # 3 add, 1 div
        case MeasureType.IDLE:
            return 0
        case _:
            return 0

def analyzeFile(filePath: str) -> Result:
    file = open(filePath, "r", encoding="unicode_escape")
    lines = file.readlines()

    usedThreads = 0
    iterationCount = 0
    arrSize = 0
    allMeasures = []

    currentMeasure: Measure = None
    for line in lines:
        splitLine = line.split(" ")
        match splitLine[0]:
            case "Tamanho": # Tamanho do array alterado para NxN
                arrSize = int(splitLine[5].split("x")[0])
            case "Quantidade":
                if splitLine[1] == "maxima": # Quantidade maxima de threads alterada para N
                    usedThreads = int(splitLine[6])
                if splitLine[1] == "de": # Quantidade de iteracoes alterado para N
                    iterationCount = int(splitLine[5])
            case "Measuring": # Measuring idle (1s)
                currentMeasure = Measure()
                currentMeasure.type = MeasureType.IDLE
                currentMeasure.executionTime = 1.0
            case "Initialize": # Initialize ALGORITHM (NxM array)
                currentMeasure = Measure()
                match splitLine[1]:
                    case "Map":
                        currentMeasure.type = MeasureType.MAP
                    case "Reduction":
                        currentMeasure.type = MeasureType.REDUCTION
                    case "Stencil":
                        currentMeasure.type = MeasureType.STENCIL
                    case _:
                        print(f"Unknown algorithm '{splitLine[1]}'")
            case "Execution": # Execution time: TIMEs
                currentMeasure.executionTime = float(splitLine[2][:-2])
            case "Package": # Package N: PKG=XJ, PP0=YJ, PP1=WJ, DRAM=ZJ
                currentPackage = PackageMeasure()
                currentPackage.pkgNumber = int(splitLine[1][0])
                for i in range(2,len(splitLine)):
                    value = float(splitLine[i].split("=")[1].split("J")[0])
                    if splitLine[i].startswith("PKG="):
                        currentPackage.pkg = value
                    elif splitLine[i].startswith("PP0="):
                        currentPackage.pp0 = value
                    elif splitLine[i].startswith("PP1="):
                        currentPackage.pp1 = value
                    elif splitLine[i].startswith("DRAM="):
                        currentPackage.dram = value
                    else:
                        print(f"Unknown RAPL domain '{splitLine[i]}'")
                currentMeasure.packages.append(currentPackage)
            case "\n":
                if currentMeasure is not None:
                    allMeasures.append(currentMeasure)
                    currentMeasure = None
    
    result = Result()
    result.fileName = pathlib.Path(file.name).stem
    result.threads = usedThreads
    result.iterations = iterationCount
    result.arraySize = arrSize
    result.measures = allMeasures
    return result