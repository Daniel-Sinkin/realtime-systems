// main.cpp
#include "types.hpp"  // IWYU pragma: keep

#include <cassert>
#include <format>
#include <limits>
#include <optional>
#include <print>
#include <random>
#include <string>
#include <vector>

namespace ds_rt
{
using TimePoint = u32;
using TimeDelta = u32;

struct JobSpec
{
    TimePoint arrival{};
    TimePoint deadline{};
    TimeDelta compute_time{};
};

enum class JobSpecValidity : u8
{
    Valid = 0,
    ZeroComputeTime,
    DeadlineBeforeArrival,
    ComputeTimeExceedsWindow,
};

[[nodiscard]] auto to_string(const JobSpec& spec) -> std::string
{
    return std::format(
        "JobSpec(arrival={}, deadline={}, compute_time={})",
        spec.arrival,
        spec.deadline,
        spec.compute_time
    );
}

[[nodiscard]] auto check_validity(const JobSpec& spec) noexcept -> JobSpecValidity
{
    if (spec.compute_time == 0)
    {
        return JobSpecValidity::ZeroComputeTime;
    }
    if (spec.deadline < spec.arrival)
    {
        return JobSpecValidity::DeadlineBeforeArrival;
    }
    if (spec.compute_time > spec.deadline - spec.arrival)
    {
        return JobSpecValidity::ComputeTimeExceedsWindow;
    }
    return JobSpecValidity::Valid;
}

[[nodiscard]] auto is_valid(const JobSpec& spec) noexcept -> bool
{
    return check_validity(spec) == JobSpecValidity::Valid;
}

struct Job
{
    JobSpec spec{};
    TimePoint start_time{};

    [[nodiscard]] auto finish_time() const noexcept -> TimePoint
    {
        return start_time + spec.compute_time;
    }
};

[[nodiscard]] auto to_string(const Job& job) -> std::string
{
    return std::format("Job(spec={}, start_time={})", to_string(job.spec), job.start_time);
}

enum class JobValidity : u8
{
    Valid = 0,
    InvalidSpec,
    StartedTooEarly,
    DeadlineMissed,
};

[[nodiscard]] auto check_validity(const Job& job) noexcept -> JobValidity
{
    if (not is_valid(job.spec))
    {
        return JobValidity::InvalidSpec;
    }
    if (job.start_time < job.spec.arrival)
    {
        return JobValidity::StartedTooEarly;
    }
    if (job.start_time > job.spec.deadline)
    {
        return JobValidity::DeadlineMissed;
    }
    if (job.spec.compute_time > job.spec.deadline - job.start_time)
    {
        return JobValidity::DeadlineMissed;
    }
    return JobValidity::Valid;
}

[[nodiscard]] auto is_valid(const Job& job) noexcept -> bool
{
    return check_validity(job) == JobValidity::Valid;
}

struct PeriodicTask
{
    TimeDelta compute_time{};
    TimeDelta period{};
    TimePoint initial_job{};  // phase / offset
    TimeDelta deadline{};
};

struct SporadicTask
{
    TimeDelta compute_time{};
    TimeDelta min_inter_arrival{};
    TimeDelta deadline{};
};

enum class PeriodicTaskValidity : u8
{
    Valid = 0,
    ZeroComputeTime,
    ZeroPeriod,
    ZeroDeadline,
    ComputeTimeExceedsDeadline,
};

enum class SporadicTaskValidity : u8
{
    Valid = 0,
    ZeroComputeTime,
    ZeroMinInterArrival,
    ZeroDeadline,
    ComputeTimeExceedsDeadline,
};

[[nodiscard]] auto to_string(const PeriodicTask& task) -> std::string
{
    return std::format(
        "PeriodicTask(compute_time={}, period={}, initial_job={}, deadline={})",
        task.compute_time,
        task.period,
        task.initial_job,
        task.deadline
    );
}

[[nodiscard]] auto to_string(const SporadicTask& task) -> std::string
{
    return std::format(
        "SporadicTask(compute_time={}, min_inter_arrival={}, deadline={})",
        task.compute_time,
        task.min_inter_arrival,
        task.deadline
    );
}

[[nodiscard]] auto check_validity(const PeriodicTask& task) noexcept -> PeriodicTaskValidity
{
    if (task.compute_time == 0)
    {
        return PeriodicTaskValidity::ZeroComputeTime;
    }
    if (task.period == 0)
    {
        return PeriodicTaskValidity::ZeroPeriod;
    }
    if (task.deadline == 0)
    {
        return PeriodicTaskValidity::ZeroDeadline;
    }
    if (task.compute_time > task.deadline)
    {
        return PeriodicTaskValidity::ComputeTimeExceedsDeadline;
    }
    return PeriodicTaskValidity::Valid;
}

[[nodiscard]] auto check_validity(const SporadicTask& task) noexcept -> SporadicTaskValidity
{
    if (task.compute_time == 0)
    {
        return SporadicTaskValidity::ZeroComputeTime;
    }
    if (task.min_inter_arrival == 0)
    {
        return SporadicTaskValidity::ZeroMinInterArrival;
    }
    if (task.deadline == 0)
    {
        return SporadicTaskValidity::ZeroDeadline;
    }
    if (task.compute_time > task.deadline)
    {
        return SporadicTaskValidity::ComputeTimeExceedsDeadline;
    }
    return SporadicTaskValidity::Valid;
}

[[nodiscard]] auto is_valid(const PeriodicTask& task) noexcept -> bool
{
    return check_validity(task) == PeriodicTaskValidity::Valid;
}

[[nodiscard]] auto is_valid(const SporadicTask& task) noexcept -> bool
{
    return check_validity(task) == SporadicTaskValidity::Valid;
}

[[nodiscard]] auto make_sporadic_rng(const std::optional<u64>& seed) -> std::mt19937_64
{
    if (seed.has_value())
    {
        return std::mt19937_64{*seed};
    }

    auto random_device = std::random_device{};
    auto seed_sequence = std::seed_seq{
        random_device(),
        random_device(),
        random_device(),
        random_device(),
    };
    return std::mt19937_64{seed_sequence};
}

[[nodiscard]] auto get_specs_from_task(const PeriodicTask& task, TimePoint t0, TimePoint t1)
    -> std::vector<JobSpec>
{
    std::vector<JobSpec> out{};
    const auto task_is_valid = is_valid(task);
    assert(task_is_valid);
    if ((not task_is_valid) or (t1 <= t0))
    {
        return out;
    }

    auto t = task.initial_job;
    if (t < t0)
    {
        const auto delta = static_cast<u64>(t0 - t);
        const auto period = static_cast<u64>(task.period);
        auto periods_to_skip = delta / period;
        if ((delta % period) != u64{0})
        {
            ++periods_to_skip;
        }

        const auto first_release = static_cast<u64>(t) + periods_to_skip * period;
        if (first_release > static_cast<u64>(std::numeric_limits<TimePoint>::max()))
        {
            return out;
        }
        t = static_cast<TimePoint>(first_release);
    }

    while (t < t1)
    {
        const auto absolute_deadline = static_cast<u64>(t) + static_cast<u64>(task.deadline);
        if (absolute_deadline > static_cast<u64>(std::numeric_limits<TimePoint>::max()))
        {
            break;
        }

        out.push_back(
            JobSpec{
                .arrival = t,
                .deadline = static_cast<TimePoint>(absolute_deadline),
                .compute_time = task.compute_time,
            }
        );

        const auto next_release = static_cast<u64>(t) + static_cast<u64>(task.period);
        if (next_release > static_cast<u64>(std::numeric_limits<TimePoint>::max()))
        {
            break;
        }
        t = static_cast<TimePoint>(next_release);
    }
    return out;
}

[[nodiscard]] auto get_specs_from_task(
    const SporadicTask& task, TimePoint t0, TimePoint t1, std::optional<u64> seed = std::nullopt
) -> std::vector<JobSpec>
{
    std::vector<JobSpec> out{};
    const auto task_is_valid = is_valid(task);
    assert(task_is_valid);
    if ((not task_is_valid) or (t1 <= t0))
    {
        return out;
    }

    auto rng = make_sporadic_rng(seed);

    const auto min_inter_arrival = static_cast<u64>(task.min_inter_arrival);
    auto first_release_offset_distribution =
        std::uniform_int_distribution<u64>{0, min_inter_arrival - 1u};
    auto inter_arrival_distribution =
        std::uniform_int_distribution<u64>{min_inter_arrival, 2u * min_inter_arrival};

    auto t = static_cast<u64>(t0) + first_release_offset_distribution(rng);
    while (t < static_cast<u64>(t1))
    {
        const auto absolute_deadline = t + static_cast<u64>(task.deadline);
        if (absolute_deadline > static_cast<u64>(std::numeric_limits<TimePoint>::max()))
        {
            break;
        }

        out.push_back(
            JobSpec{
                .arrival = static_cast<TimePoint>(t),
                .deadline = static_cast<TimePoint>(absolute_deadline),
                .compute_time = task.compute_time,
            }
        );

        const auto next_release = t + inter_arrival_distribution(rng);
        if (next_release > static_cast<u64>(std::numeric_limits<TimePoint>::max()))
        {
            break;
        }
        t = next_release;
    }
    return out;
}

}  // namespace ds_rt

auto main() -> int
{
    using namespace ds_rt;

    /*mut*/ TimePoint time{0};
    const TimePoint time_completion{10};

    while (time < time_completion)
    {
        ++time;
    }
    std::println("Finished Running");
}
