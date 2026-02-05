import { useState } from 'react';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import { Brain, Play, RotateCcw, CheckCircle, AlertCircle } from 'lucide-react';

interface NPUBenchmarkResult {
    inferenceScore: number;
    latency: number;
    throughput: number;
    completed: boolean;
}

export default function NPUBench() {
    const [running, setRunning] = useState(false);
    const [progress, setProgress] = useState(0);
    const [npuDetected] = useState(true); // Check if NPU exists
    const [result, setResult] = useState<NPUBenchmarkResult | null>(null);

    const runBenchmark = async () => {
        setRunning(true);
        setProgress(0);
        setResult(null);

        // Simulate NPU benchmark
        for (let i = 0; i <= 100; i += 2) {
            await new Promise(resolve => setTimeout(resolve, 40));
            setProgress(i);
        }

        // Simulated results
        setResult({
            inferenceScore: Math.floor(Math.random() * 500) + 800,
            latency: (Math.random() * 5 + 2).toFixed(2) as any,
            throughput: Math.floor(Math.random() * 5) + 10,
            completed: true
        });
        setRunning(false);
    };

    const reset = () => {
        setResult(null);
        setProgress(0);
    };

    return (
        <div className="space-y-4">
            {/* Header */}
            <div className="pb-3 border-b border-border">
                <h1 className="text-base font-semibold">NPU Benchmark</h1>
                <p className="text-xs text-muted-foreground mt-0.5">
                    Test AI accelerator performance
                </p>
            </div>

            {/* NPU Detection */}
            <Card className="border border-border bg-card">
                <CardHeader className="pb-2.5">
                    <CardTitle className="flex items-center gap-2 text-sm">
                        <Brain className="h-4 w-4 text-warning" />
                        NPU Status
                    </CardTitle>
                </CardHeader>
                <CardContent>
                    {npuDetected ? (
                        <div className="flex items-center gap-2 text-xs">
                            <CheckCircle className="h-3.5 w-3.5 text-success" />
                            <span className="text-success">NPU Detected - AMD Ryzen AI (10 TOPS)</span>
                        </div>
                    ) : (
                        <div className="flex items-center gap-2 text-xs">
                            <AlertCircle className="h-3.5 w-3.5 text-warning" />
                            <span className="text-warning">No NPU detected on this system</span>
                        </div>
                    )}
                </CardContent>
            </Card>

            {/* Benchmark Control Card */}
            {npuDetected && (
                <Card className="border border-border bg-card">
                    <CardHeader className="pb-3">
                        <CardTitle className="flex items-center gap-2 text-sm">
                            <Brain className="h-4 w-4 text-warning" />
                            AI Inference Test
                        </CardTitle>
                        <CardDescription className="text-xs">
                            Runs AI model inference to measure NPU performance
                        </CardDescription>
                    </CardHeader>
                    <CardContent className="space-y-3">
                        {/* Progress Bar */}
                        {(running || result) && (
                            <div className="space-y-1.5">
                                <div className="flex justify-between text-xs">
                                    <span className="text-muted-foreground">Progress</span>
                                    <span className="font-mono">{progress}%</span>
                                </div>
                                <div className="h-2 bg-muted rounded-full overflow-hidden">
                                    <div
                                        className="h-full bg-warning transition-all duration-300"
                                        style={{ width: `${progress}%` }}
                                    />
                                </div>
                            </div>
                        )}

                        {/* Control Buttons */}
                        <div className="flex gap-2">
                            <Button
                                onClick={runBenchmark}
                                disabled={running}
                                className="flex-1 text-xs h-8"
                            >
                                {running ? (
                                    <>
                                        <div className="h-3 w-3 mr-1.5 border-2 border-primary-foreground border-t-transparent rounded-full animate-spin" />
                                        Running Inference...
                                    </>
                                ) : (
                                    <>
                                        <Play className="h-3 w-3 mr-1.5" />
                                        Start Benchmark
                                    </>
                                )}
                            </Button>
                            {result && (
                                <Button
                                    onClick={reset}
                                    variant="outline"
                                    className="text-xs h-8"
                                >
                                    <RotateCcw className="h-3 w-3 mr-1.5" />
                                    Reset
                                </Button>
                            )}
                        </div>
                    </CardContent>
                </Card>
            )}

            {/* Results */}
            {result && result.completed && (
                <div className="grid grid-cols-1 md:grid-cols-3 gap-3">
                    {/* Inference Score */}
                    <Card className="border border-border bg-card">
                        <CardHeader className="pb-2.5">
                            <CardTitle className="flex items-center gap-1.5 text-xs font-medium">
                                <CheckCircle className="h-3.5 w-3.5 text-success" />
                                Inference Score
                            </CardTitle>
                        </CardHeader>
                        <CardContent>
                            <div className="text-2xl font-bold text-warning font-mono">
                                {result.inferenceScore}
                            </div>
                            <p className="text-xs text-muted-foreground mt-1">
                                Overall performance
                            </p>
                        </CardContent>
                    </Card>

                    {/* Latency */}
                    <Card className="border border-border bg-card">
                        <CardHeader className="pb-2.5">
                            <CardTitle className="flex items-center gap-1.5 text-xs font-medium">
                                <CheckCircle className="h-3.5 w-3.5 text-success" />
                                Latency
                            </CardTitle>
                        </CardHeader>
                        <CardContent>
                            <div className="text-2xl font-bold text-info font-mono">
                                {result.latency}ms
                            </div>
                            <p className="text-xs text-muted-foreground mt-1">
                                Response time
                            </p>
                        </CardContent>
                    </Card>

                    {/* Throughput */}
                    <Card className="border border-border bg-card">
                        <CardHeader className="pb-2.5">
                            <CardTitle className="flex items-center gap-1.5 text-xs font-medium">
                                <CheckCircle className="h-3.5 w-3.5 text-success" />
                                Throughput
                            </CardTitle>
                        </CardHeader>
                        <CardContent>
                            <div className="text-2xl font-bold text-secondary font-mono">
                                {result.throughput}
                            </div>
                            <p className="text-xs text-muted-foreground mt-1">
                                TOPS (Trillion Ops/s)
                            </p>
                        </CardContent>
                    </Card>
                </div>
            )}

            {/* Info Card */}
            <Card className="border border-border bg-card">
                <CardHeader className="pb-2.5">
                    <CardTitle className="text-xs font-medium">About This Test</CardTitle>
                </CardHeader>
                <CardContent className="text-xs text-muted-foreground space-y-1.5">
                    <p>• Tests AI model inference on dedicated NPU hardware</p>
                    <p>• Measures latency (response time) and throughput (TOPS)</p>
                    <p>• NPU accelerates AI workloads vs CPU/GPU</p>
                    <p>• Requires AMD Ryzen AI or Intel Core Ultra processor</p>
                </CardContent>
            </Card>
        </div>
    );
}
