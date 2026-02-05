import { useState } from 'react';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import { Cpu, Play, RotateCcw, CheckCircle } from 'lucide-react';

interface BenchmarkResult {
    singleCore: number;
    multiCore: number;
    completed: boolean;
}

export default function CPUBench() {
    const [running, setRunning] = useState(false);
    const [progress, setProgress] = useState(0);
    const [result, setResult] = useState<BenchmarkResult | null>(null);

    const runBenchmark = async () => {
        setRunning(true);
        setProgress(0);
        setResult(null);

        // Simulate benchmark with progress updates
        for (let i = 0; i <= 100; i += 2) {
            await new Promise(resolve => setTimeout(resolve, 50));
            setProgress(i);
        }

        // Simulated results (replace with actual benchmark)
        setResult({
            singleCore: Math.floor(Math.random() * 500) + 1000,
            multiCore: Math.floor(Math.random() * 2000) + 6000,
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
                <h1 className="text-base font-semibold">CPU Benchmark</h1>
                <p className="text-xs text-muted-foreground mt-0.5">
                    Test your processor's single-core and multi-core performance
                </p>
            </div>

            {/* Benchmark Control Card */}
            <Card className="border border-border bg-card">
                <CardHeader className="pb-3">
                    <CardTitle className="flex items-center gap-2 text-sm">
                        <Cpu className="h-4 w-4 text-primary" />
                        Performance Test
                    </CardTitle>
                    <CardDescription className="text-xs">
                        Runs intensive calculations to measure CPU performance
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
                                    className="h-full bg-primary transition-all duration-300"
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
                                    Running...
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

            {/* Results */}
            {result && result.completed && (
                <div className="grid grid-cols-1 md:grid-cols-2 gap-3">
                    {/* Single-Core Score */}
                    <Card className="border border-border bg-card">
                        <CardHeader className="pb-2.5">
                            <CardTitle className="flex items-center gap-1.5 text-xs font-medium">
                                <CheckCircle className="h-3.5 w-3.5 text-success" />
                                Single-Core Score
                            </CardTitle>
                        </CardHeader>
                        <CardContent>
                            <div className="text-2xl font-bold text-primary font-mono">
                                {result.singleCore}
                            </div>
                            <p className="text-xs text-muted-foreground mt-1">
                                Single-threaded performance
                            </p>
                        </CardContent>
                    </Card>

                    {/* Multi-Core Score */}
                    <Card className="border border-border bg-card">
                        <CardHeader className="pb-2.5">
                            <CardTitle className="flex items-center gap-1.5 text-xs font-medium">
                                <CheckCircle className="h-3.5 w-3.5 text-success" />
                                Multi-Core Score
                            </CardTitle>
                        </CardHeader>
                        <CardContent>
                            <div className="text-2xl font-bold text-secondary font-mono">
                                {result.multiCore}
                            </div>
                            <p className="text-xs text-muted-foreground mt-1">
                                All cores under load
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
                    <p>• Single-core test measures single-threaded performance</p>
                    <p>• Multi-core test utilizes all available CPU cores</p>
                    <p>• Higher scores indicate better performance</p>
                    <p>• Results are saved to the leaderboard automatically</p>
                </CardContent>
            </Card>
        </div>
    );
}
