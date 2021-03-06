/*
 * =====================================================================================
 *
 *       Filename: pathfinder.hpp
 *        Created: 03/28/2017 17:04:54
 *    Description: A-Star algorithm for path finding
 *
 *                 if we have a prefperence, we should make it by the cost function,
 *                 rather than hack the source to make a priority
 *
 *                 support jump on the map with step size as (1, 2, 3)
 *                 store direction information in each node and calculate cost with it
 *
 *
 *        Version: 1.0
 *       Revision: none
 *       Compiler: gcc
 *
 *         Author: ANHONG
 *          Email: anhonghe@gmail.com
 *   Organization: USTC
 *
 * =====================================================================================
 */

#pragma once
#include <tuple>
#include <cmath>
#include <array>
#include <vector>
#include <functional>

#include "fsa.h"
#include "stlastar.h"
#include "totype.hpp"
#include "strf.hpp"
#include "fflerror.hpp"
#include "condcheck.hpp"
#include "protocoldef.hpp"

namespace PathFind
{
    enum
    {
        INVALID = 0, OBSTACLE, OCCUPIED, LOCKED, FREE,
    };

    struct PathNode final
    {
        int X;
        int Y;

        PathNode(int nX = -1, int nY = -1)
            : X(nX)
            , Y(nY)
        {}

        bool operator == (const PathNode & rstNode)
        {
            return (rstNode.X == X) && (rstNode.Y == Y);
        }

        bool Eq(int nX, int nY) const
        {
            return X == nX && Y == nY;
        }
    };

    template<size_t R, size_t N = 3> class GridMaskMap final
    {
        private:
            int m_X;
            int m_Y;

        private:
            static constexpr int m_gridBit       = (int)(N);
            static constexpr int m_gridBitMask   = (1 << m_gridBit) - 1;
            static constexpr int m_gridPerInt    = sizeof(int) * 8 / m_gridBit;
            static constexpr int m_gridLineWidth = ((2 * R - 1) + (m_gridPerInt - 1)) / m_gridPerInt;

        private:
            std::array<int, (2 * R - 1) * m_gridLineWidth> m_buf;

        public:
            GridMaskMap(int nCenterX, int nCenterY)
                : m_X(nCenterX - (int)(R) + 1)
                , m_Y(nCenterY - (int)(R) + 1)
                , m_buf()
            {
                static_assert(R > 0, "Requires a non-zero radius");
                static_assert(N > 0, "Requires a non-zero grid bit-width");
                m_buf.fill(0);
            }

        public:
            int GetGrid(int nX, int nY) const
            {
                int nDX = nX - m_X;
                if(nDX < 0 || nDX >= (int)(2 * R - 1)){
                    return 0;
                }

                int nDY = nY - m_Y;
                if(nDY < 0 || nDY >= (int)(2 * R - 1)){
                    return 0;
                }

                int nOff   = (nDX / m_gridPerInt) + m_gridLineWidth * nDY;
                int nShift = (nDX % m_gridPerInt) * m_gridBit;
                return (m_buf[nOff] >> nShift) & m_gridBitMask;
            }

            void SetGrid(int nX, int nY, int nVal)
            {
                int nDX = nX - m_X;
                if(nDX < 0 || nDX >= (int)(2 * R - 1)){
                    return;
                }

                int nDY = nY - m_Y;
                if(nDY < 0 || nDY >= (int)(2 * R - 1)){
                    return;
                }

                int nOff   = (nDX / m_gridPerInt) + m_gridLineWidth * nDY;
                int nShift = (nDX % m_gridPerInt) * m_gridBit;

                m_buf[nOff] |= (        m_gridBitMask  << nShift);
                m_buf[nOff] &= ((nVal & m_gridBitMask) << nShift);
            }
    };

    inline bool ValidDir(int nDirection)
    {
        return  nDirection >= DIR_BEGIN && nDirection < DIR_END;
    }

    inline const char *GetDirName(int nDirection)
    {
        switch (nDirection){
            case DIR_NONE      : return "DIR_NONE";
            case DIR_UP        : return "DIR_UP";
            case DIR_DOWN      : return "DIR_DOWN";
            case DIR_LEFT      : return "DIR_LEFT";
            case DIR_RIGHT     : return "DIR_RIGHT";
            case DIR_UPLEFT    : return "DIR_UPLEFT";
            case DIR_UPRIGHT   : return "DIR_UPRIGHT";
            case DIR_DOWNLEFT  : return "DIR_DOWNLEFT";
            case DIR_DOWNRIGHT : return "DIR_DOWNRIGHT";
            default            : return "DIR_UNKNOWN";
        }
    }

    inline int GetBack(int nDirection)
    {
        switch (nDirection){
            case DIR_UP        : return DIR_DOWN;
            case DIR_DOWN      : return DIR_UP;
            case DIR_LEFT      : return DIR_RIGHT;
            case DIR_RIGHT     : return DIR_LEFT;
            case DIR_UPLEFT    : return DIR_DOWNRIGHT;
            case DIR_UPRIGHT   : return DIR_DOWNLEFT;
            case DIR_DOWNLEFT  : return DIR_UPRIGHT;
            case DIR_DOWNRIGHT : return DIR_UPLEFT;
            default            : return DIR_NONE;
        }
    }

    inline void GetFrontLocation(int *pX, int *pY, int nX, int nY, int nDirection, int nLen = 1)
    {
        static constexpr int nDX[] = { 0, +1, +1, +1,  0, -1, -1, -1};
        static constexpr int nDY[] = {-1, -1,  0, +1, +1, +1,  0, -1};

        if(nDirection < DIR_BEGIN || nDirection >= DIR_END){
            throw fflerror("In PathFind::GetFrontLocation(%p, %p, %d, %d, %d, %d)", to_cvptr(pX), to_cvptr(pY), nX, nY, nDirection, nLen);
        }

        if(pX){
            *pX = nX + nLen * nDX[nDirection - DIR_BEGIN];
        }

        if(pY){
            *pY = nY + nLen * nDY[nDirection - DIR_BEGIN];
        }
    }

    inline void GetBackLocation(int *pX, int *pY, int nX, int nY, int nDirection, int nLen = 1)
    {
        GetFrontLocation(pX, pY, nX, nY, GetBack(nDirection), nLen);
    }

    // return direction for src -> dst
    // direction code defined in protocoldef.hpp
    inline int GetDirection(int nSrcX, int nSrcY, int nDstX, int nDstY)
    {
        static constexpr int nDirV[][3]
        {
            {DIR_UPLEFT,   DIR_UP,   DIR_UPRIGHT  },
            {DIR_LEFT,     DIR_NONE, DIR_RIGHT    },
            {DIR_DOWNLEFT, DIR_DOWN, DIR_DOWNRIGHT},
        };

        int nDX = (nDstX > nSrcX) - (nDstX < nSrcX);
        int nDY = (nDstY > nSrcY) - (nDstY < nSrcY);

        return nDirV[nDY + 1][nDX + 1];
    }

    int MaxReachNode(const PathFind::PathNode *, size_t, size_t);
}

class AStarPathFinderNode;
class AStarPathFinder: public AStarSearch<AStarPathFinderNode>
{
    private:
        friend class AStarPathFinderNode;

    private:
        const std::function<double(int, int, int, int)> m_oneStepCost;

    private:
        const int m_maxStep;

    private:
        bool m_foundPath;

    public:
        AStarPathFinder(std::function<double(int, int, int, int)> fnOneStepCost, int nMaxStepSize = 1)
            : AStarSearch<AStarPathFinderNode>()
            , m_oneStepCost(fnOneStepCost)
            , m_maxStep(nMaxStepSize)
            , m_foundPath(false)
        {
            condcheck(m_oneStepCost);
            condcheck(false
                    || (m_maxStep == 1)
                    || (m_maxStep == 2)
                    || (m_maxStep == 3));
        }

    public:
        ~AStarPathFinder()
        {
            if(m_foundPath){
                FreeSolutionNodes();
            }
            EnsureMemoryFreed();
        }

    protected:
        int MaxStep() const
        {
            return m_maxStep;
        }

    public:
        bool PathFound() const
        {
            return m_foundPath;
        }

    public:
        inline bool Search(int, int, int, int);

    public:
        inline std::vector<PathFind::PathNode> GetPathNode();

    public:
        template<size_t PathNodeNum> std::tuple<std::array<PathFind::PathNode, PathNodeNum>, size_t> GetFirstNPathNode();
};

class AStarPathFinderNode
{
    public:
        friend class AStarPathFinder;
        friend class AStarSearch<AStarPathFinderNode>;

    private:
        int m_X;
        int m_Y;

    private:
        // direction when *stopping* at current place
        // direction map: 0 -> DIR_UP
        //                1 -> DIR_UPRIGHT
        //                2 -> DIR_RIGHT
        //                3 -> DIR_DOWNRIGHT
        //                4 -> DIR_DOWN
        //                5 -> DIR_DOWNLEFT
        //                6 -> DIR_LEFT
        //                7 -> DIR_UPLEFT
        //
        // if mark m_direction as -1 (invalid)
        // means we don't take consider of turn cost
        int m_direction;

    private:
        AStarPathFinder *m_finder;

    private:
        AStarPathFinderNode() = default;
        AStarPathFinderNode(int nX, int nY, int nDirection, AStarPathFinder *pFinder)
            : m_X(nX)
            , m_Y(nY)
            , m_direction(nDirection)
            , m_finder(pFinder)
        {
            condcheck(pFinder);
        }

    public:
        ~AStarPathFinderNode() = default;

    public:
        int X() const
        {
            return m_X;
        }

        int Y() const
        {
            return m_Y;
        }

        int Direction() const
        {
            return m_direction;
        }

    public:
        float GoalDistanceEstimate(const AStarPathFinderNode &rstGoalNode) const
        {
            // we use Chebyshev's distance instead of Manhattan distance
            // since we allow max step size as 1, 2, 3, and for optimal solution

            // to make A-star algorithm admissible
            // we need h(x) never over-estimate the distance

            auto nMaxStep   = (m_finder->MaxStep());
            auto nXDistance = (std::labs(rstGoalNode.X() - X()) + (nMaxStep - 1)) / nMaxStep;
            auto nYDistance = (std::labs(rstGoalNode.Y() - Y()) + (nMaxStep - 1)) / nMaxStep;

            return (std::max<float>)(nXDistance, nYDistance);
        }

        bool IsGoal(const AStarPathFinderNode &rstGoalNode) const
        {
            return (X() == rstGoalNode.X()) && (Y() == rstGoalNode.Y());
        }

        bool GetSuccessors(AStarSearch<AStarPathFinderNode> *pAStarSearch, const AStarPathFinderNode *pParentNode) const
        {
            static const int nDX[] = { 0, +1, +1, +1,  0, -1, -1, -1};
            static const int nDY[] = {-1, -1,  0, +1, +1, +1,  0, -1};

            for(int nStepIndex = 0; nStepIndex < ((m_finder->MaxStep() > 1) ? 2 : 1); ++nStepIndex){
                for(int nDirIndex = 0; nDirIndex < 8; ++nDirIndex){
                    int nNewX = X() + nDX[nDirIndex] * ((nStepIndex == 0) ? m_finder->MaxStep() : 1);
                    int nNewY = Y() + nDY[nDirIndex] * ((nStepIndex == 0) ? m_finder->MaxStep() : 1);

                    if(true
                            && pParentNode
                            && pParentNode->X() == nNewX
                            && pParentNode->Y() == nNewY){
                        continue;
                    }

                    // when add a successor we always check it's distance between the ParentNode
                    // means for m_moveChecker(x0, y0, x1, y1) we guarentee that (x1, y1) inside propor distance to (x0, y0)

                    if(m_finder->m_oneStepCost(X(), Y(), nNewX, nNewY) >= 0.00){
                        AStarPathFinderNode stFinderNode {nNewX, nNewY, nDirIndex, m_finder};
                        pAStarSearch->AddSuccessor(stFinderNode);
                    }
                }
            }

            // seems we have to always return true
            // return false means out of memory in the code
            return true;
        }

        // give very close weight for StepSize = 1 and StepSize = MaxStep to prefer bigger hops
        // but I have to make Weight(MaxStep) = Weight(1) + dW ( > 0 ), reason:
        //       for MaxStep = 3 and path as following:
        //                       A B C D E
        //       if I want to move (A->C), we can do (A->B->C) and (A->D->C)
        //       then if there are of same weight I can't prefer (A->B->C)
        //
        //       but for MaxStep = 2 I don't have this issue
        // actually for dW < Weight(1) is good enough

        // cost :   valid  :     1.00
        //        occupied :   100.00
        //         invalid : 10000.00
        //
        // we should have cost(invalid) >> cost(occupied), otherwise
        //          XXAXX
        //          XOXXX
        //          XXBXX
        // path (A->O->B) and (A->X->B) are of equal cost

        // if can't go through we return the infinite
        // be careful of following situation which could make mistake
        //
        //     XOOAOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO
        //     XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXO
        //     XOOBOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO
        //
        // here ``O" means ``can pass" and ``X" means not, then if we do move (A->B)
        // if the path is too long then likely it takes(A->X->B) rather than (A->OOOOOOO...OOO->B)
        //
        // method to solve it:
        //  1. put path length constraits
        //  2. define inifinite = Map::W() * Map::H() as any path can have

        float GetCost(const AStarPathFinderNode &rstNode) const
        {
            int nSrcX = X();
            int nSrcY = Y();
            int nDstX = rstNode.X();
            int nDstY = rstNode.Y();

            auto fCost = m_finder->m_oneStepCost(nSrcX, nSrcY, nDstX, nDstY);
            condcheck(fCost >= 0.00);

            if(Direction() < 0){
                return (float)(fCost);
            }

            // need to add turn cost
            // current node has direction info

            auto nOldDirIndex = Direction();
            auto nNewDirIndex = PathFind::GetDirection(nSrcX, nSrcY, nDstX, nDstY) - (DIR_NONE + 1);
            condcheck(true
                    && (nOldDirIndex >= 0) && (nOldDirIndex < 8)
                    && (nNewDirIndex >= 0) && (nNewDirIndex < 8));

            auto nDDirIndex = ((nNewDirIndex - nOldDirIndex) + 8) % 8;
            return (float)(fCost) + 1.00 * (std::min<int>)(nDDirIndex, 8 - nDDirIndex);
        }

        bool IsSameState(const AStarPathFinderNode &rstNode) const
        {
            return (X() == rstNode.X()) && (Y() == rstNode.Y());
        }
};

inline bool AStarPathFinder::Search(int nX0, int nY0, int nX1, int nY1)
{
    AStarPathFinderNode stNode0 {nX0, nY0, -1, this};
    AStarPathFinderNode stNode1 {nX1, nY1, -1, this};

    SetStartAndGoalStates(stNode0, stNode1);

    unsigned int nSearchState;
    do{
        nSearchState = SearchStep();
    }while(nSearchState == AStarSearch<AStarPathFinderNode>::SEARCH_STATE_SEARCHING);

    m_foundPath = (nSearchState == AStarSearch<AStarPathFinderNode>::SEARCH_STATE_SUCCEEDED);
    return m_foundPath;
}

inline std::vector<PathFind::PathNode> AStarPathFinder::GetPathNode()
{
    if(!PathFound()){
        return {};
    }

    std::vector<PathFind::PathNode> stPathRes;
    if(auto pNode0 = GetSolutionStart()){
        stPathRes.emplace_back(pNode0->X(), pNode0->Y());
    }else{
        return {};
    }

    while(auto pNode = GetSolutionNext()){
        stPathRes.emplace_back(pNode->X(), pNode->Y());
    }

    return stPathRes;
}

template<size_t PathNodeNum> std::tuple<std::array<PathFind::PathNode, PathNodeNum>, size_t> AStarPathFinder::GetFirstNPathNode()
{
    static_assert(PathNodeNum >= 2, "PathFinder::GetFirstNPathNode(): template argument invalid");
    std::array<PathFind::PathNode, PathNodeNum> stPathRes;

    // some C++ question to myself:
    // why I can't directly return {{}, 0} ? what's implicit constructible?

    if(!PathFound()){
        return {stPathRes, 0};
    }

    if(auto pNode0 = GetSolutionStart()){
        stPathRes[0] = {pNode0->X(), pNode0->Y()};
    }else{
        return {stPathRes, 0};
    }

    for(size_t nIndex = 1; nIndex < stPathRes.size(); ++nIndex){
        if(auto pNode = GetSolutionNext()){
            stPathRes[nIndex] = {pNode->X(), pNode->Y()};
        }else{
            return {stPathRes, nIndex};
        }
    }
    return {stPathRes, stPathRes.size()};
}
